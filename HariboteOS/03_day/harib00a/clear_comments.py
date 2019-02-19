from tempfile import mkstemp
from shutil import move
from os import fdopen, remove
import re
import sys

def replace(file_path, pattern, subst):
    #Create temp file
    fh, abs_path = mkstemp()
    with fdopen(fh,'w') as new_file:
        with open(file_path) as old_file:
            for index, line in enumerate(old_file):
            	if (index > 3):
                	new_file.write(re.sub(pattern, subst, line))
                else:
                	new_file.write(line)
    #Remove original file
    remove(file_path)
    #Move new file
    move(abs_path, file_path)

replace(sys.argv[1], ';(.*)','')