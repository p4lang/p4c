# SDE version number.

# Note added Jan. 25 2023: as per Steve Licking, this version is no longer to be incremented,
#                          not even the "patch_version".
major_version = 8
minor_version = 6
patch_version = 0


def get_code_version():
    return "%s.%s.%s" % (str(major_version), str(minor_version), str(patch_version))
