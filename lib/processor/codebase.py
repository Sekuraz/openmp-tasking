
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

    def transform(self):
        for file in self.files:
            file.transform()

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


class BRACE_TYPE:
    class __bt:
        def __init__(self, start, end):
            self.start = start
            self.end = end
    DEFAULT = __bt('(', ')')
    CURLY = __bt('{', '}')


def get_next_brace(string, start, brace=BRACE_TYPE.DEFAULT, ret='str'):
    index = start
    while string[index] != brace.start:
        index += 1
    code = ''
    braces = 1
    index += 1
    while braces > 0:
        char = string[index]
        if char == brace.start:
            braces += 1
        if char == brace.end:
            braces -= 1
        code += char
        index += 1
    if ret == 'str':
        return code[:-1]
    elif ret == 'int':
        return index


def get_next_block(string, start, ret='str'):
    """
    Only simple blocks.
    Either statements or curly braces are valid. No keywords at the start!
    :param string:
    :param start:
    :param ret:
    :return:
    """
    while string[start] in ' \t\n\r':
        start += 1

    index = start
    if string[index] == '{':
        index = get_next_brace(string, index, brace=BRACE_TYPE.CURLY, ret='int')
    else:
        while string[index] != ';':
            index += 1

    if ret == 'str':
        return string[start:index]
    elif ret == 'int':
        return index


def skip_until(string, start, pattern, except_pattern=False):
    end = start
    while string[end] in pattern:
        end += 1
    return end


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
        for match in re.finditer('#pragma omp task[^\w]', self.content):
            index = match.start()
            text = self.content[index : self.content.find('\n', index)]
            index += len(text)
            while text[-1] == '\\':
                text = text[:-1]
                new = self.content[index: self.content.find('\n', index + 1)]
                index += len(new) + 1
                text += new
            text = re.sub('\s+', ' ', text)

            task = Task(text, match.start())
            task.parse()

            task.code = self.get_task_code(index + 1)
            print(self.path + '     ' + str(task.start))
            print(task.source)
            task.code.parse()

            self.tasks.append(task)
        if 'test.' in self.path:
            pass

    def get_task_code(self, start):
        index = start
        index = skip_until(self.content, index, ' \t\n\r')  # skip whitespace

        if self.content[index] == '{':
            code = get_next_brace(self.content, index, BRACE_TYPE.CURLY)
            return TaskCode(code, index)
        else:
            i = index
            kw = ''
            while self.content[i].isalpha():
                kw += self.content[i]
                i += 1
            if kw in ['if', 'for', 'switch', 'while']:
                i = get_next_brace(self.content, i, ret='int')
                i = get_next_block(self.content, i, ret='int')
                return TaskCode(self.content[index:i], index)
            elif kw == 'do':
                i = get_next_block(self.content, i, ret='int')
                i = get_next_brace(self.content, i, ret='int')

                i = self.content.find(';', i)

                return TaskCode(self.content[index:i], index)
            elif kw == 'try':
                raise NotImplementedError("Starting a task with 'try' is not supported. Wrap it in curly braces!")
            else:
                i = self.content.find(';', i)
                return TaskCode(self.content[index:i + 1], index)

    def transform(self):
        if self.has_main:
            self._transform_main()

    def _transform_main(self):
        pass

    @property
    def has_main(self):
        if self.__has_main is None:
            for reg in config.main_regexes:
                if re.search(reg, self.content):
                    self.__has_main = True
                    break
            else:
                self.__has_main = False
        return self.__has_main

    def __str__(self):
        ret = "File(path=%s, tasks=%s)" % (self.path, self.tasks)
        return ret

    def __repr__(self):
        return self.path


class Task:
    def __init__(self, source, start):
        self.source = source
        self.start = start
        self.code = None

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

    def parse(self):
        if 'if' in self.source:
            code = get_next_brace(self.source, self.source.find('if'))
            self.IF = re.sub('task\s*:', '', code[:-1])
        if 'final' in self.source:
            self.final = get_next_brace(self.source, self.source.find('final'))
        if 'untied' in self.source:
            self.untied = True
        if 'default' in self.source:
            self.default = get_next_brace(self.source, self.source.find('default'))
        if 'mergeable' in self.source:
            self.mergeable = True
        if ',private' in self.source or ' private' in self.source:
            index = self.source.find(',private')
            if index < 0:
                index = self.source.find(' private')
            data = get_next_brace(self.source, index).split(',')
            self.private = [item.strip() for item in data]
        if 'firstprivate' in self.source:
            data = get_next_brace(self.source, self.source.find('firstprivate')).split(',')
            self.firstprivate = [item.strip() for item in data]
        if 'shared' in self.source:
            data = get_next_brace(self.source, self.source.find('shared')).split(',')
            self.shared = [item.strip() for item in data]
        # depend
        if 'depend' in self.source:
            print(self.source)
        if 'priority' in self.source:
            self.priority = int(get_next_brace(self.source, self.source.find('priority')))

    def __str__(self):
        ret = 'Task('
        for k, v in self.__dict__.items():
            if k == 'source':
                continue
            if k == 'default' and v == 'shared':
                continue
            if v or isinstance(v, int):
                if isinstance(v, str) and ' ' in v:
                    ret += '%s=(%s), ' % (k.lower(), v)
                else:
                    ret += '%s=%s, ' % (k.lower(), v)

        if ret[-1] == ' ':
            ret = ret[:-2]
        ret += ')'
        return ret

    __repr__ = __str__


class TaskCode:
    def __init__(self, source, start):
        self.source = source
        self.start = start

        self.read = []
        self.write = []
        self.access = []
        self.defines = []

    def parse(self):
        pass
