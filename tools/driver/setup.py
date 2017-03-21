import p4c
import os

from setuptools import setup, find_packages

# support python path/to/setup.py install
os.chdir(os.path.dirname(os.path.abspath(__file__)))

setup (
    name = "p4c",
    version = p4c.__version__,

    author = p4c.__author__,
    license = "BSD",
    description = "P4 compiler driver",
    long_description = """\
TODO(hanw)
""",
    packages = find_packages(),
    package_data = {"": ["*.cfg"]},
    entry_points = {
        'console_scripts': [
            'p4c = p4c:main',
            ],
        }
)
