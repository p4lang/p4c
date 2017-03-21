import os

# recursive find, good for developer
def rec_find_bin(cwd, exe):
    found = None
    for root, dirs, files in os.walk(cwd):
        for f in files:
            if f == exe:
                return root + '/' + f
    cwd = os.path.abspath(os.path.join(cwd, os.pardir))
    if cwd != "/":
        found = rec_find_bin(cwd, exe)
    return found

def use_rec_find(exe):
    cwd = os.getcwd()
    return rec_find_bin(cwd, exe)

def getLocalCfg(config):
    """
    Search bottom-up for p4c.site.cfg
    """
    return use_rec_find(config)

# top-down find, good for deployment
def find_bin(exe):
    found_path = None
    for pp in os.environ['PATH'].split(':'):
        for root, dirs, files in os.walk(pp):
            for ff in files:
                if ff == exe:
                    found_path = pp + '/' + ff
    return found_path

