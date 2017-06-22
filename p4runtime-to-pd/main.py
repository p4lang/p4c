#! /usr/bin/python

import argparse
import collections
import os
import re
import sys

import p4info_pb2
import v1model_pb2

from tenjin_wrapper import render_template

_TENJIN_PREFIX = "//::"  # Use // in prefix for C syntax processing

_THIS_DIR = os.path.dirname(os.path.realpath(__file__))

_TEMPLATES_DIR = os.path.join(_THIS_DIR, "p4runtime-to-pd/templates")

TABLES = {}
TABLES_BY_ID = {}
ACTIONS = {}
ACTIONS_BY_ID = {}
ACT_PROFS = {}
ACT_PROFS_BY_ID = {}
COUNTER_ARRAYS = {}
COUNTER_ARRAYS_BY_ID = {}
METER_ARRAYS = {}
METER_ARRAYS_BY_ID = {}
LEARN_QUANTAS = {}
LEARN_QUANTAS_BY_ID = {}
REGISTER_ARRAYS = {}
REGISTER_ARRAYS_BY_ID = {}

def sort_dict(unsorted_dict):
    return collections.OrderedDict(sorted(unsorted_dict.items(),
                                          key=lambda item: item[0]))


def get_p4_14_name(name):
    # XXX(seth): This is a bit of a hack. We want to strip off most of the
    # hierarchy introduced by the conversion to P4-16, so we remove everything
    # except the last dot-separated component.
    n = name.rsplit('.', 1)[-1]

    return n


def get_p4_14_field_name(name):
    # XXX(seth): This is a bit of a hack. We want to strip off most of the
    # hierarchy introduced by the conversion to P4-16. Even in P4-14, though,
    # field names do include a dot, so we remove every dot-separated component
    # except the last two.
    n = '.'.join(name.rsplit('.', 2)[-2:])
    n = n.replace(".$valid$", ".valid")

    return n


def get_c_name(name):
    # Rewrite characters that are invalid in C identifiers.
    n = name.replace(".", "_")
    n = n.replace("[", "_")
    n = n.replace("]", "_")

    return n


def enum(type_name, *sequential, **named):
    enums = dict(zip(sequential, range(len(sequential))), **named)
    reverse = dict((value, key) for key, value in enums.iteritems())
    # enums['reverse_mapping'] = reverse

    @staticmethod
    def to_str(x):
        return reverse[x].lower()
    enums['to_str'] = to_str

    @staticmethod
    def from_str(x):
        return enums[x.upper()]
    enums['from_str'] = from_str

    return type(type_name, (), enums)


def protobuf_enum(type_name, protobuf_enum_type):
    enums = dict(protobuf_enum_type.items())
    reverse = dict((value, key) for key, value in enums.iteritems())
    # enums['reverse_mapping'] = reverse

    @staticmethod
    def to_str(x):
        return reverse[x].lower()
    enums['to_str'] = to_str

    @staticmethod
    def from_str(x):
        return enums[x.upper()]
    enums['from_str'] = from_str

    return type(type_name, (), enums)


MatchType = protobuf_enum('MatchType', p4info_pb2.MatchField.MatchType)
TableType = enum('TableType', 'SIMPLE', 'INDIRECT', 'INDIRECT_WS')
MeterUnit = protobuf_enum('MeterUnit', p4info_pb2.MeterSpec.Unit)
#MeterType = protobuf_enum('MeterType', p4info_pb2.MeterSpec.Type)
MeterType = protobuf_enum('MeterUnit', p4info_pb2.MeterSpec.Unit)


class Table:
    def __init__(self, name, id_):
        self.name = get_p4_14_name(name)
        self.id_ = id_
        self.match_type = None
        self.type_ = None
        self.action_prof = None  # for indirect tables only
        self.actions = {}
        self.key = []
        self.default_action = None
        self.with_counters = False
        self.direct_meters = None
        self.support_timeout = False

        TABLES[self.name] = self
        TABLES_BY_ID[id_] = self

    @property
    def cname(self):
        return get_c_name(self.name)

    def set_match_type(self):
        assert(self.match_type is None)
        match_types = [t for _, t, _ in self.key]

        if len(match_types) == 0:
            self.match_type = MatchType.EXACT
        elif MatchType.RANGE in match_types:
            self.match_type = MatchType.RANGE
        elif MatchType.TERNARY in match_types:
            self.match_type = MatchType.TERNARY
        elif match_types.count(MatchType.LPM) > 1:
            print "cannot have 2 different lpm matches in a single table"
            sys.exit(1)
        elif MatchType.LPM in match_types:
            self.match_type = MatchType.LPM
        else:
            # that includes the case when we only have one valid match and
            # nothing else
            self.match_type = MatchType.EXACT

    def num_key_fields(self):
        return len(self.key)

    def key_str(self):
        def one_str(f):
            name, t, bw = f
            return name + "(" + MatchType.to_str(t) + ", " + str(bw) + ")"

        return ",\t".join([one_str(f) for f in self.key])

    def table_str(self):
        return "{0:30} [{1}]".format(self.name, self.key_str())

    def __str__(self):
        return self.table_str()


class Action:
    def __init__(self, name, id_):
        self.name = get_p4_14_name(name)
        self.id_ = id_
        self.runtime_data = []

        ACTIONS[self.name] = self
        ACTIONS_BY_ID[id_] = self

    @property
    def cname(self):
        return get_c_name(self.name)

    def num_params(self):
        return len(self.runtime_data)

    def runtime_data_str(self):
        return ",\t".join([name + "(" + str(bw) + ")"
                           for name, bw in self.runtime_data])

    def action_str(self):
        return "{0:30} [{1}]".format(self.name, self.runtime_data_str())


class ActProf:
    def __init__(self, name, id_, with_selector):
        self.name = get_p4_14_name(name)
        self.id_ = id_
        self.with_selector = with_selector
        self.with_selection = with_selector
        self.actions = {}

        ACT_PROFS[self.name] = self
        ACT_PROFS_BY_ID[id_] = self

    @property
    def cname(self):
        return get_c_name(self.name)


class CounterArray:
    def __init__(self, name, id_, is_direct, table=""):
        self.name = get_p4_14_name(name)
        self.id_ = id_
        self.is_direct = is_direct
        self.table = table

        COUNTER_ARRAYS[self.name] = self
        COUNTER_ARRAYS_BY_ID[id_] = self

    @property
    def cname(self):
        return get_c_name(self.name)

    def counter_str(self):
        return "{0:30} [{1}]".format(self.name, self.is_direct)


class MeterArray:
    # hacks to make them more easily accessible in template
    MeterUnit = MeterUnit
    MeterType = MeterType

    def __init__(self, name, id_, is_direct, unit, type_, table=""):
        self.name = get_p4_14_name(name)
        self.id_ = id_
        self.is_direct = is_direct
        #self.unit = unit
        #self.type_ = type_
        self.type_ = unit
        self.table = table

        METER_ARRAYS[self.name] = self
        METER_ARRAYS_BY_ID[id_] = self

    @property
    def cname(self):
        return get_c_name(self.name)

    def meter_str(self):
        return "{0:30} [{1}, {2}]".format(self.name, self.is_direct,
                                          MeterType.to_str(self.type_))

    def __str__(self):
        return self.meter_str()


class LearnQuanta:
    def __init__(self, name, id_):
        self.name = get_p4_14_name(name)
        self.id_ = id_
        self.fields = []  # (name, bitwidth)

        LEARN_QUANTAS[self.name] = self
        LEARN_QUANTAS_BY_ID[id_] = self

    @property
    def cname(self):
        return get_c_name(self.name)

    def field_str(self):
        return ",\t".join([name + "(" + str(bw) + ")"
                           for name, bw in self.fields])

    def lq_str(self):
        return "{0:30} [{1}]".format(self.name, self.field_str())


def analyze_p4info(p4info):
    TABLES.clear()
    TABLES_BY_ID.clear()
    ACTIONS.clear()
    ACTIONS_BY_ID.clear()
    ACT_PROFS.clear()
    ACT_PROFS_BY_ID.clear()
    COUNTER_ARRAYS.clear()
    COUNTER_ARRAYS_BY_ID.clear()
    METER_ARRAYS.clear()
    METER_ARRAYS_BY_ID.clear()
    LEARN_QUANTAS.clear()
    LEARN_QUANTAS_BY_ID.clear()

    no_action_id = -1

    for action in p4info.actions:
        # Skip NoAction, but remember its id so we can recognize references to
        # it later.
        if get_p4_14_name(action.preamble.name) == 'NoAction':
            no_action_id = action.preamble.id
            continue

        pd_action = Action(action.preamble.name, action.preamble.id)
        for param in action.params:
            pd_action.runtime_data += [(get_p4_14_name(param.name),
                                        param.bitwidth)]

    for table in p4info.tables:
        pd_table = Table(table.preamble.name, table.preamble.id)
        pd_table.type_ = TableType.SIMPLE
        for action_ref in table.action_refs:
            if action_ref.id == no_action_id: continue
            pd_action = ACTIONS_BY_ID[action_ref.id]
            if pd_action.name != 'NoAction':
                pd_table.actions[pd_action.name] = pd_action
        for match_field in table.match_fields:
            pd_table.key += [(get_p4_14_field_name(match_field.name),
                              match_field.match_type,
                              match_field.bitwidth)]
        pd_table.set_match_type()
        if table.with_entry_timeout:
            pd_table.support_timeout = True

    for action_profile in p4info.action_profiles:
        pd_tables = [TABLES_BY_ID[id] for id in action_profile.table_ids]
        if not pd_tables:  # should not happen
            continue

        pd_action_profile = ActProf(action_profile.preamble.name,
                                    action_profile.preamble.id,
                                    action_profile.with_selector)

        # All tables with the same action profile should have the same actions,
        # so we chose one arbitrarily to collect the actions from.
        pd_action_profile.actions = pd_tables[0].actions

        # Update table types.
        for pd_table in pd_tables:
            pd_table.action_prof = pd_action_profile

            assert(pd_table.type_ == TableType.SIMPLE)
            if pd_action_profile.with_selector:
                pd_table.type_ = TableType.INDIRECT_WS
            else:
                pd_table.type_ = TableType.INDIRECT

    for counter in p4info.counters:
        counter = CounterArray(counter.preamble.name, counter.preamble.id, False)

    for counter in p4info.direct_counters:
        pd_table = TABLES_BY_ID[counter.direct_table_id]
        counter = CounterArray(counter.preamble.name, counter.preamble.id, True,
                               pd_table.name)
        pd_table.with_counters = True

    for meter in p4info.meters:
        pd_meter = MeterArray(meter.preamble.name, meter.preamble.id, \
                              False, meter.spec.unit, meter.spec.type)

    for meter in p4info.direct_meters:
        pd_table = TABLES_BY_ID[meter.direct_table_id]
        pd_meter = MeterArray(meter.preamble.name, meter.preamble.id, \
                              True, meter.spec.unit, meter.spec.type,
                              pd_table.name)
        pd_table.direct_meters = pd_meter.name

    for extern in p4info.externs:
        if extern.extern_type_name != 'digest':
            continue
        for instance in extern.instances:
            assert(instance.info.TypeName() == 'p4.config.v1model.Digest')
            digest = v1model_pb2.Digest()
            instance.info.Unpack(digest)

            # XXX(seth): We're mapping all uses of the digest primitive to
            # LearnQuanta here, but does that cover all uses of digest?
            pd_learn_quanta = LearnQuanta(instance.preamble.name,
                                          instance.preamble.id)
            for field in digest.fields:
                pd_learn_quanta.fields.append((field.name, field.bitwidth))


def ignore_template_file(filename):
    """
    Ignore these files in template dir
    """
    pattern = re.compile('^\..*|.*\.cache$|.*~$')
    return pattern.match(filename)


def gen_file_lists(current_dir, gen_dir):
    """
    Generate target files from template; only call once
    """
    files_out = []
    for root, subdirs, files in os.walk(current_dir):
        for filename in files:
            if ignore_template_file(filename):
                continue
            relpath = os.path.relpath(os.path.join(root, filename), current_dir)
            template_file = relpath
            target_file = os.path.join(gen_dir, relpath)
            files_out.append((template_file, target_file))
    return files_out


def render_all_files(render_dict, gen_dir, templates_dir):
    files = gen_file_lists(templates_dir, gen_dir)
    for template, target in files:
        path = os.path.dirname(target)
        if not os.path.exists(path):
            os.makedirs(path)
        with open(target, "w") as f:
            render_template(f, template, render_dict, templates_dir,
                            prefix=_TENJIN_PREFIX)


def _validate_dir(dir_name):
    if not os.path.isdir(dir_name):
        print dir_name, "is not a valid directory"
        sys.exit(1)
    return os.path.abspath(dir_name)


def get_c_type(byte_width):
    if byte_width == 1:
        return "uint8_t"
    elif byte_width == 2:
        return "uint16_t"
    elif byte_width <= 4:
        return "uint32_t"
    else:
        return "uint8_t *"


# key is a Python list of tuples (field_name, match_type, bitwidth)
def gen_match_params(key):
    params = []
    for field, match_type, bitwidth in key:
        bytes_needed = bits_to_bytes(bitwidth)
        if match_type == MatchType.RANGE:
            params += [(field + "_start", bytes_needed)]
            params += [(field + "_end", bytes_needed)]
        else:
            params += [(field, bytes_needed)]
        if match_type == MatchType.LPM:
            params += [(field + "_prefix_length", 2)]
        if match_type == MatchType.TERNARY:
            params += [(field + "_mask", bytes_needed)]
    return params


def gen_action_params(runtime_data):
    params = []
    for name, bitwidth in runtime_data:
        # for some reason, I was prefixing everything with "action_" originally
        name = "action_" + name
        params += [(name, bits_to_bytes(bitwidth))]
    return params


def bits_to_bytes(bw):
    return (bw + 7) / 8


def get_thrift_type(byte_width):
    if byte_width == 1:
        return "byte"
    elif byte_width == 2:
        return "i16"
    elif byte_width <= 4:
        return "i32"
    elif byte_width == 6:
        return "MacAddr_t"
    elif byte_width == 16:
        return "IPv6_t"
    else:
        return "binary"


def generate_pd_source(dest_dir, p4_prefix, templates_dir, target):
    for table in TABLES.itervalues():
        table.actions = sort_dict(table.actions)

    for profile in ACT_PROFS.itervalues():
        profile.actions = sort_dict(profile.actions)

    render_dict = {}
    render_dict["p4_prefix"] = p4_prefix
    render_dict["pd_prefix"] = "p4_pd_" + p4_prefix + "_"
    render_dict["MatchType"] = MatchType
    render_dict["TableType"] = TableType
    render_dict["MeterUnit"] = MeterUnit  # new
    render_dict["MeterType"] = MeterType
    render_dict["gen_match_params"] = gen_match_params
    render_dict["gen_action_params"] = gen_action_params
    render_dict["bits_to_bytes"] = bits_to_bytes
    render_dict["get_c_type"] = get_c_type
    render_dict["get_c_name"] = get_c_name
    render_dict["get_thrift_type"] = get_thrift_type
    render_dict["tables"] = sort_dict(TABLES)
    render_dict["actions"] = sort_dict(ACTIONS)
    render_dict["learn_quantas"] = sort_dict(LEARN_QUANTAS) # not handled
    render_dict["action_profs"] = sort_dict(ACT_PROFS)  # action_profs in the other version
    render_dict["counter_arrays"] = sort_dict(COUNTER_ARRAYS)
    render_dict["meter_arrays"] = sort_dict(METER_ARRAYS)
    render_dict["register_arrays"] = sort_dict(REGISTER_ARRAYS) # not handled
    render_dict["render_dict"] = render_dict

    if target == "bm":
        render_dict["target_common_h"] = "<bm/pdfixed/pd_common.h>"
    elif target == "tofino":
        render_dict["target_common_h"] = "<tofino/pdfixed/pd_common.h>"
    else:
        assert(0)

    render_all_files(render_dict, _validate_dir(dest_dir), templates_dir)


def _validate_dir(path):
    path = os.path.abspath(path)
    if not os.path.isdir(path):
        print path, "is not a valid directory"
        sys.exit(1)
    return path


def main(templates_dir=_TEMPLATES_DIR):
    parser = argparse.ArgumentParser(description='P4Runtime PD frontend generator')
    parser.add_argument('source', metavar='p4runtime', type=str,
                        help='A P4Runtime configuration in the default '
                             'binary protobuf format.')
    parser.add_argument('--pd', dest='pd', type=str,
                        help='Generate PD C code from the P4Runtime '
                        'configuration in this directory. The directory '
                        'must already exist.',
                        required=True)
    parser.add_argument('--p4-prefix', type=str,
                        help='P4 name to use for API function prefixes.',
                        default="prog", required=False)
    # This is temporary, need to make things uniform
    parser.add_argument('--target', type=str, choices=["bm", "tofino"],
                        help='Target for which the PD frontend is generated.',
                        default="bm", required=False)

    args = parser.parse_args()

    path_pd = _validate_dir(args.pd)

    print "Reading P4Runtime from", args.source
    with open(args.source, 'rb') as f:
        p4info = p4info_pb2.P4Info()
        p4info.ParseFromString(f.read())

    print "Analyzing P4Runtime"
    analyze_p4info(p4info)

    print "Generating PD frontend source files in", path_pd
    generate_pd_source(path_pd, args.p4_prefix, templates_dir, args.target)


if __name__ == "__main__":
    main()
