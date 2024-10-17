from __future__ import print_function
import unicodedata, sys

SUCCESS = 0
FAILURE = 1

# ----------------------------------------
#  Helpers
# ----------------------------------------

def print_error_and_exit(error_msg):
    print(error_msg, file=sys.stderr)
    sys.exit(FAILURE)


def next_power_2(n):
    p = 0
    while (2**p) < n:
        p += 1
    return 2**p

# Returns True if the current version is >= version need
def version_check_ge(version_need, current_version):
    cur = current_version.split(".")
    need = version_need.split(".")
    if len(cur) == len(need):
        try:
            for i, c in enumerate(cur):  # Iterate through major, minor1, minor2
                if int(c) < int(need[i]):
                    return False
                elif int(c) > int(need[i]):  # Major/minor1/minor2 is greater, stop looking
                    break
        except ValueError:
            return False
    else:
        return False
    return True


def has_attr(attr, node):
    if attr in node:
        return True
    return False


def get_attr(attr, node):
    if attr in node:
        if sys.version_info.major == 2:
            v = node[attr]
            if isinstance(v, unicode):
                return unicodedata.normalize('NFKD', v).encode('ascii', 'ignore')
            return node[attr]
        else:
            return node[attr]
    print_error_and_exit("Unable to find attribute '%s' at node '%s'" % (str(attr), str(node)))


def get_optional_attr(attr, node):
    if has_attr(attr, node):
        return get_attr(attr, node)
    return None


def get_attr_first_available(attr, node, first_available_version, current_version):
    if version_check_ge(first_available_version, current_version):
        return get_attr(attr, node)
    return None


def get_optional_attr_first_available(attr, node, first_available_version, current_version):
    if version_check_ge(first_available_version, current_version):
        return get_optional_attr(attr, node)
    return None


def list2str(some_list):
    if len(some_list) == 0:
        return "-"
    return ", ".join(some_list)


def obj2str(some_obj):
    if some_obj is None:
        return ""
    return str(some_obj)


def print_table(column_headers, data_values):
    """
    Returns a string that represents a pretty printed table.

    :param column_headers: a list of lists for the column names.
    :param data_values: a list of lists for the values in the columns.
    """
    if len(data_values) == 0:
        return ""

    def get_m(column_index):
        if len(data_values) > 5000:
            return 22
        max_name = max([len(str(x[column_index])) for x in column_headers])
        max_value = max([len(str(x[column_index])) for x in data_values])
        return max(max_name + 2, max_value + 2)

    def get_row(values):
        row_ = ""
        for idx, value in enumerate(values):
            value_as_string = str(value)
            row_ += "|%s" % value_as_string.center(get_m(idx))

        row_ += "|\n"
        return row_

    to_return = ""

    max_string = 0

    for a_header in column_headers:
        hdr = get_row(a_header)
        if len(hdr) > max_string:
            max_string = len(hdr)
        to_return += hdr

    spacer = "-"
    while len(spacer) < max_string:
        spacer += "-"

    to_return += "%s\n" % spacer

    for data_value in data_values:
        to_return += get_row(data_value)

    return "%s\n%s%s\n" % (spacer, to_return, spacer)
