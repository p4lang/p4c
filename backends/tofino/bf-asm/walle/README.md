Walle - JSON-to-binary cruncher tool
====================================================

Walle serves as a layer of abstraction between the Tofino compiler and chip,
presenting the chip's memory hierarchy to the compiler as a set of JSON
structures that contain register/memory names and their values, while
abstracting away the actual addresses of these registers and the methods by
which they are programmed (DMA/direct PCIe writes/indirect instruction lists).

Walle stores the exact structure of the chip's memory hierarchy in a
"chip.schema" file, which has to be generated from raw register data whenever
the chip registers change, and is then used afterwards to crunch compiler output
into a binary config file. It can also be used to generate "template" JSON that
looks like compiler output with the hardware's default values for all fields (in
most cases, 0). These templates are used by the compiler to enforce the correct
structure on its output data, and also in general should be regenerated whenever
the chip's registers change.

Using Walle
----------------------------------------------------
### Basic usage
#### Generating a schema
First, generate a chip schema. Invoke Walle with the `--generate-schema` flag
followed by the directory containing raw CSV files output by csrCompiler. If
the bfnregs repo is cloned into ~/bfnregs, this would be:

    ./walle.py --generate-schema ~/bfnregs/modules/tofino_regs/module/csv/

This will generate a file named `chip.schema` in the current working directory,
which is where it will look for the chip schema by default. The
`--schema SCHEMA-FILE` flag can be used to point Walle to a different schema,
or a different location to output the schema it is generating.

#### Crunching compiler output
The most common use case for Walle is taking multiple config JSONs and
crunching them into a binary file Tofino's drivers can read. Just invoke
Walle with the names of all relevant JSON files, and optionally the name of
the file to output:

    ./walle.py cfg1.json cfg2.json cfg3.json -o chip_config.bin

If the compiler was set up to dump all of its config output into an otherwise
empty directory, shell wildcards can be used to shorten this command. If that
dir is called 'cfgs', this would look like:

    ./walle.py cfgs/*.json -o chip_config.bin

#### Generating templates
Walle can be used to generate blank register templates to be filled in by the
compiler. These templates are the JSON files that Walle would expect to see
given the current chip schema, but with all of the data set to the corresponding
hardware register's power-on default (in most cases, 0).

To do so, Walle must be fed a JSON file enumerating the Semifore addressmap
objects it should generate templates for. This file must take this structure:

   {
     "generated": {
         "memories":[
            // memory addressmap names
         ],
         "regs":[
            // register addressmap names
         ]
     },
     "ignored": {
         "memories":[
            // memory addressmap names
         ],
         "regs":[
            // register addressmap names
         ]
      }
   }

Names under 'memories' keys refer to addressmaps included by the top-level
'pipe_top_level.csr' file, while names under 'regs' keys refer to those included
by 'tofino.csr'.

Address maps listed under 'generated' will cause a JSON template file to be
generated. Wherever that address map appears elsewhere in the hierarchy will be
replaced with a string reference to said JSON file.

Address maps listed under 'ignored' will be replaced with a 0 when they appear 
elsewhere in the hierarchy, and no JSON template file will be generated.

Use the `--generate-templates` flag followed by the path to a file of the
format just discussed to generate template JSONs in a directory called
`templates` (which will be created in the working directory if it doesn't
already exist):

    ./walle.py --generate-templates templates_file

These files can then be copied to the compiler's source tree.

The templates themselves end in the extension '.cfg.json'. Walle will also
generate an identical hierarchy containing the bit-widths of each field, and
these files end in the extension '.size.json'.

### Advanced usage
#### Directing the crunch process
Walle crunches by first loading all provided JSON files and verifying them
against its chip schema, and then drilling down from specified "top-level"
points in that cloud of JSON data. By default, these points are called
`memories.top` and `regs.top` and represent the memory and register hierarchies
of the chip, respectively.

The `--top NAME` flag can be used to manually specify the top-level points to
drill down from. Multiple `--top NAME` flags can be included, and if any are
present the default top-level names are not used.

This is equivalent to the default behavior:

    ./walle.py cfgs/*.json --top memories.top --top regs.top -o chip_config.bin

One of them can be left out to only generate, say, only register configuration:

    ./walle.py cfgs/*.json --top regs.top -o chip_config.bin

Walle calculates addresses relative to the top-level points specified, so it is
important that these points only ever refer to actual top-level points in the
Semifore register hierarchy. If it is desired to only generate, for example,
config data for the MAU or one pipe, the top-level JSON files should be
hand-tweaked to disable other parts of the configuration binary. See the
specification of the JSON config format for more details.

#### Directing the template generation process
Walle generates a template file for each addressmap type specified in the
`template_objects` file which sits in the same folder as the Walle script. If
Walle encounters an instance of these addressmap types during template
generation, it leaves that tree of the JSON data unexpanded and replaces it
with a string indicating it expects a template to be plugged in to that
location.

The type names of these addressmaps can be found by viewing the Semifore HTML
output of the reg and memory hierarchies and checking 'Header File Information'
at the top of the page. The 'Type Name' field that then appears within each
address map indicates the type which should be passed to Walle for
templatization. Semifore incorrectly capitlizes the first letter of the type
name - it should be all lowercase when specified to Walle.

Note that the JSON fed to Walle does *not* have to follow the same template
structure as specified in the `template_objects` file - this templatization
control is just for convenience and reducing the file size of the generated
blank templates.

Configuration JSON format
----------------------------------------------------
Walle consumes JSON files that specify values to be written registers named in
the chip's Semifore specification. The structure of these JSON files directly
mirrors the structure found in the chip's Semifore specification.

Each JSON file contains a dictionary that represents one instance of a Semifore
addressmap. Addressmap dictionaries' keys represent the Semifore names of
registers and nested addressmaps, while the values are either:

   *  Dictionaries representing those objects
   *  Lists of dictionaries, in the case the object in question is an array
   *  Lists of lists (of lists of lists of lists of...) in the case the object
      in question is an N-dimensional array

Register dictionaries have field names as keys and integers as values. They
follow the same rules for lists in the event of a field array. The outer-most
dictionary also has these special Walle keys:        

   *  `_type` : The full type name that this file provides values for, of the
      form `section.semifore_type`. For example, the parser's memories are of
      type `memories.prsr_mem_rspec`, while its registers are of type
     `regs.prsr_reg_rspec`
   *  `_name` : A name used to reference this file and its data elsewhere in
      the config JSON
   *  `_schema_hash`: The MD5 hash of the raw Semifore output used to generate
      the chip schema from which this file's structure was derived, used
      to ensure the chip schema and JSON input match
   *  `_reg_version`: The git tag of the bfnregs repo commit used to generate
      the chip schema from which this file's structure was derived. This value
      isn't used by Walle itself, but is useful to manually determine which
      version of the compiler or model a given config JSON was created for.

At any point in the hierarchy, a register/addressmap value may be replaced
with:

   *  A string containing the name of another JSON input file, which "stamps"
      that other data down at this point in the memory hierarchy
   *  0, indicating no write operation should be generated for the given object

Fields cannot be "disabled with 0s" the way registers and addressmaps can,
since the register is the level of granularity at which the drivers write data.

Config JSON can be hand-tweaked with 0's to produce a binary blob that
only writes to specific registers and leaves everything else alone, in order
to produce "initial boot" config blobs and then "soft reboot" config blobs.

#### Error checking
Walle will fail to generate output if:

   *  A field value ever exceeds the field's bit width as specified in the chip
     schema
   *  A template is instantiated at a point in the hierarchy that does not
     match the type expected by the chip schema (eg, naming an instance of
     `memories.prsr_mem_rspec` in the top-level *register* JSON)
   *  A file's `_schema_hash` value does not match the hash stored in the chip
     schema. This check can be suppressed with the flag
     `--ignore-schema-mismatch`:

        ./walle.py cfgs/*.json --ignore-schema-mismatch -o chip_config.bin

     This flag is provided for development purposes, because even a small
     change at one end of the register hierarchy (like correcting a typo in a
     register *description*) will change the hash without actually affecting
     the structure of the chip schema, and it would be a pain to have to
     regenerate all templates and copy them over into the compiler source tree
     just to get things working again.

     In the long run, however, this flag should not be used and schema hashes
     should be consistent.

Binary blob format
----------------------------------------------------
Walle generates a sequence of binary write instructions for the driver which
are of the following types:

   *  Direct register write - For 32 bit registers that can be addressed
      directly from the PCIe bus, a simple address-data pair of the form:

            4 bytes: "\0\0\0R"
            4 bytes: 32-bit PCIe address
            4 bytes: Data
            All fields little-endian

   *  Indirect register write - For registers wider than 32 bits, or to compose
      many direct register writes into one write list that can be transmitted
      across PCIe as a single transaction.

      TODO: not actually implemented driver-or-Walle-side yet, since the model
      doesn't currently support indirect reg addressing

   *  DMA block write - Automatically chosen for arrays of registers larger
      than 4 elements, a base address and block of data:

            4 bytes: "\0\0\0D"
            8 bytes: 42-bit chip address
            4 bytes: Bit-length of word
            4 bytes: Number of words
            Following: Data, in 32-bit word chunks
            All fields little-endian

        TODO: currently only registers in the 'memories' half of the hierarchy
        will get rolled into DMA blocks, again because the model doesn't
        currently support indirect reg addressing. Eventually this won't
        be a problem

The driver should execute these instructions in the order they are read. The
binary blob has no header or structure aside from these write instructions,
so multiple binary files can be concatenated together or split into parts as
needed.

Walle can be optionally instructed to generate a direct register write to
address 0xFFFFFFFF at the very end of the file to signify to the model the end
of configuration data. This is enabled with the flag `--append-sentinel`:

    ./walle.py cfgs/*.json --append-sentinel -o chip_config.bin

