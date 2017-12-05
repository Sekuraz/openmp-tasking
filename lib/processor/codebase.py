
import os
import re

from . import config


class CodeBase:
    def __init__(self, directory):
        directory = os.path.realpath(directory)
        assert(os.path.exists(directory))
        self.directory = directory

        self.files = []

    def parse(self):
        for root, _, files in os.walk(self.directory):
            for f in files:
                if not is_code(f):
                    continue

                file = File(os.path.join(root, f))
                self.files.append(file)

        for file in self.files:
            file.parse()

    def __str__(self):
        ret = "CodeBase(directory=%s, files=%s)" % (self.directory, self.files)
        return ret


def is_code(path):
    code = False
    for ext in path.split('.'):
        if ext in config.extensions_ignore:
            return False
        if ext in config.extensions:
            code = True
    return code


class File:
    def __init__(self, path):
        path = os.path.relpath(path)

        assert(os.path.exists(path))
        self.path = path
        self.tasks = []

        with open(self.path, 'r') as f:
            self.content = f.read()

        self.__has_main = None

    def parse(self):
        passx

    @property
    def has_main(self):
        if self.__has_main is None:
            for reg in config.main_regexes:
                if re.search(reg, self.content):
                    m = re.search('(' + reg + ')', self.content)
                    print(m.group(0), m.start())
                    self.__has_main = True
                    break
            else:
                self.__has_main = False
        return self.__has_main

    def __str__(self):
        return self.path

    __repr__ = __str__

class Task:
    def __init__(self):
        self.IF = None #str #if false, parent may not continue until this is finished
        self.final = None #str # if true: sequential, also for children
        self.untied = False #ignore, continue on same node, schedule on any
        self.default = 'shared' #ignore: (shared | none) - are values shared by default or do they have to have a clause for it
        self.mergeable = False #ignore
        self.private = [] #all variables by default, no write back necessary
        self.firstprivate = [] #as above, but initialized with value before, no write back necessary
        self.shared = [] #copy in/out, dev responsible for sync
        self.IN = [] #list of strings, runtime ordering for siblings, array sections?
        self.OUT = []
        self.INOUT = []
        self.priority = 0 #later