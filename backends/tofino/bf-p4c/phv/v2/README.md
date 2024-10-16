# PHV Allocation V2
## Understand a super cluster
A SuperCluster is a closure of field slices that must be allocated to a container group,
because of PARDE, MAU and potentially pragma-induced constraints.

## Allocation workflow
```ascii
        SUPERCLUSTER
             │
             │MAX
    ┌────────┼──────────┐
    │        │          │
    ▼        ▼          ▼
 SLICING1 SLICING2     ...  // [Slicing]
             │
             │SUM
    ┌────────┼──────────┐
    │        │          │
    ▼        ▼          ▼
   SC1      SC2        ...
             │
             │MAX
    ┌────────┼──────────┐
    │        │          │
    ▼        ▼          ▼
ALIGNMENT1 ALIGNMENT2  ...  // [Alignment]
             |
             │MAX
    ┌────────┼──────────┐
    │        │          │
    ▼        ▼          ▼
  GROUP1   GROUP2      ...  // [(SC, ScAllocAlignment)*GROUP]
             │
             │SUM
             ▼
    ┌───────────────────┐
    │                   │
    ▼                   ▼
SLICELIST        ROTATIONAL_CLUSTER(S)
    │                   │
    │                   │SUM
    │          ┌────────┴──────────┐
    │          │                   │
    │          ▼                   ▼
    │    ALIGNED_CLUSTER1    ALIGNED_CLUSTER2...
    │          │
    │          │
    │          │
    │          │
    │MAX       │MAX OF DIFFERENT START
    ▼          ▼

     DIFFERENT CONTAINER(s)     // [Container]
```
## Allocator primitives
### Super cluster formation
#### Slice list
#### Rotational cluster
#### Aligned Cluster
### Gress
### Bit-in-byte alignment
### Max container bytes
### Exact containers
### Mutex
### Physical liverange
### Extracted
### Uninitialized read
### PaContainerType
### PaContainerSize
### No holes
### Solitary
### PaNoPack
### Deparser-zero
## Hardware constraints
### PHV
#### Gress
#### Implicit initialization
### Parser
#### Extract
##### Extracted together
##### Write mode
#### Loop and header stack
### Deparser
#### Emit
#### Mirror
#### Learning
#### Pktgen
#### Resubmit
### TM
#### Instrinsic metadata
### MAU
#### Move-based instruction
#### Bitwise and whole-container instruction
#### Wide arithmetics instruction
#### SALU instruction
#### Funnel-shift
Merged in one supercluster through no_holes
## Pragma constraints

## Known issue(s)
### Complicated action packing
### Parser multiple-write fields are solitary but packed in slice lists.