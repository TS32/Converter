def hex2dec(hexdata):
    return int(hexdata, 16)

#get the hex data from the file and convert it to decimal
def get_hex_data(filename):
    with open(filename, 'r') as f:
        hexdata = f.read()
    return hexdata  

#get a column from the hex data 
def get_column(hexdata, column):
    column_data = ''
    for i in range(0, len(hexdata), 2):
        column_data += hexdata[i+column]
    return column_data

#get a column from the text file
def get_column_text(filename, column):
    column_data = ''
    with open(filename, 'r') as f:
        for line in f:
            column_data += line[column]
    return column_data


