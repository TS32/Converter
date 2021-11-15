import os
import sys
'''dataConverter used to convert any text file to binary file or binary file to text file'''

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

def Txt2Bin(txtfile,binfile=None):
    '''
    Open a text file and convert it to binary

    '''
    if binfile is None:
        binfile = txtfile.split('.')[0]+'.bin'
    with open(txtfile,'r') as f:
        with open(binfile,'wb') as b:
            for line in f: # encode the text file to binary
                for num in line.split():
                    b.write(int(num,10).to_bytes(1,'big')) 
            b.close()
        f.close()            
    return binfile

def addLeadingSpace(string,length):
    '''
    Add leading space to a string

    '''
    return ' '*(length-len(string))+string

def getIntegerCount(txtfile):
    '''Count the number of integers in a text file'''
    with open(txtfile,'r') as f:
        count = 0
        for line in f:
            for num in line.split():
                count+=1
        f.close()
    return count


def Bin2Txt(binfile,txtfile=None,SEP='  ',count=16):
    '''
    Open a binary file and convert it to text

    '''
    filesize = os.path.getsize(binfile)
    if txtfile is None:
        txtfile = binfile.split('.')[0]+'.txt'  # if no txt file is specified, use the same name as the bin file
    read=0
    with open(binfile,'rb') as f:
        with open(txtfile,'w') as b:
            while(read<filesize):
                txt = str(int.from_bytes(f.read(1),'big'))
                b.write(addLeadingSpace(txt,3))
                b.write(SEP)
                read+=1
                if not read%count:
                    b.write("\n")
            b.close()        
        f.close()
    return txtfile  # return the name of the txt file

def printBinfile(binfile,count=16,maxLine=256):   # print the hexadecimal representation of the binary file
    '''
    Print the hexadecimal representation of the binary file in console with a line break after each line

    '''
    print("filename:"+binfile)
    filesize = os.path.getsize(binfile)
    print("fileSize:"+str(filesize)+" bytes")
    print("hexadecimal representation of the binary file:")    
    read=0
    with open(binfile,'rb') as f:
        while(read<filesize):
            for i in range(count):                
                print("0X"+(f.read(1).hex()).upper(),end=' ')                
                read+=1
                if read>=filesize:
                    break            
            print("\n",end='')
        f.close()
    print("\n")

def printTxtFile(txtfile,maxLine=256):
    '''
    Print the text file in console

    '''
    print("filename:"+txtfile)
    filesize = os.path.getsize(txtfile)
    integerCount = getIntegerCount(txtfile)

    print("fileSize:"+str(filesize)+" bytes")
    print("integerCount:"+str(integerCount))
    print("text file:")    
    with open(txtfile,'r') as f:
        for i,line in  enumerate(f):
            if i > maxLine:
                break
            print(line,end='')
        print("\n")
        f.close()
    print("\n")

def getFileShortName(filename):
    '''
    Get the short name of a file

    '''
    return os.path.basename(filename)
 
    
if __name__ == '__main__':
    if len(sys.argv) == 2:
        srcfile = getFileShortName(sys.argv[1])
        if(srcfile.endswith('.txt')):# convert txt file to binary file if the file name ends with .txt
            print("converting txt file %s to binary file %s..."%(srcfile,srcfile.split('.')[0]+'.bin'))
            printTxtFile(srcfile)
            printBinfile((Txt2Bin(srcfile)))
        elif(srcfile.endswith('.bin')): # convert binary file to txt file if the file name ends with .bin
            print("converting binary file %1 to txt file %s..."%(srcfile,srcfile.split('.')[0]+'.txt'))
            printBinfile(srcfile)
            printTxtFile(Bin2Txt(srcfile))
        else: # if the file name does not end with .txt or .bin, print an error message
            print('Error: The file name must end with .txt or .bin')
    
    elif len(sys.argv) == 3:
        srcfile = getFileShortName(sys.argv[1])
        dstfile = getFileShortName(sys.argv[2])
        if(srcfile.endswith('.txt') and dstfile.endswith('.bin')): # convert txt file to binary file if the file name ends with .txt
            print("converting txt file %s to binary file %s..."%(srcfile,dstfile))
            printTxtFile(srcfile)
            printBinfile((Txt2Bin(srcfile,dstfile)))            
        elif(srcfile.endswith('.bin') and dstfile.endswith('.txt')): # convert binary file to txt file if the file name ends with .bin
            print("converting binary file %s to txt file %s..."%(srcfile,dstfile))
            printBinfile(srcfile)
            printTxtFile(Bin2Txt(srcfile,dstfile))
        else: # if the file name does not end with .txt or .bin, print an error message 
            print('Error: The file name must end with .txt or .bin')
    else:
        print("Usage:dataConverter.py <filename.txt> [<filename.bin>]")
        print("Convert a text file to binary")        
        print("Usage: dataConverter.py <filename.bin> [<filename.txt>]")
        print("Convert a binary file to text")  








