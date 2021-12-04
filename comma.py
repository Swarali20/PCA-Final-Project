filepath_in = 'web-Google.txt'
filepath_out = 'web-Google_change.txt'
with open(filepath_in, 'r') as file_in, open(filepath_out, 'w') as file_out:
    for line in file_in:
        new_line = line.replace('\t', ',')
        file_out.write(new_line) 