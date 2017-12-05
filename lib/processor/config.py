
extensions = ['c', 'h', 'cpp', 'hpp']
extensions_ignore = ['gch']
main_regexes = [
    'int[\n\r\s]+main[\n\r\s]*\([\n\r\s]*(void)?[\n\r\s]*\)',
    'int[\n\r\s]+main[\n\r\s]*\([\n\r\s]*int[\n\r\s]+\w+[\n\r\s]*,[\n\r\s]*char[^\)]*\)',
]