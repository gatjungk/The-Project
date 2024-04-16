# Use of read and write to a function from:
# https://www.freecodecamp.org/news/python-write-to-file-open-read-append-and-other-file-handling-functions-explained/

def check_similarity(file_path1, file_path2):
    # Open two different binary files and comparing the contents for differences
    with open(file_path1, 'rb') as file1, open(file_path2, 'rb') as file2:
        file1_contents = file1.read()
        file2_contents = file2.read()
        # Compare file contents, if same return 1
        return file1_contents == file2_contents

file_path1 = 'output.bin'
file_path2 = 'compare.bin'

if check_similarity(file_path1, file_path2):
    print("The files are identical.")
else:
    print("The files are not identical.")
