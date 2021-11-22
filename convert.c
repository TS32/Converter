#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include <windows.h>
#include <Commdlg.h>

//Add link to the library, during compile and link, please add option '-l comdlg32' to the linker
#pragma comment(lib, "comdlg32.lib")

/*Read numbers from text file and convert them to hex, then store to a binary file */
int txt2bin(const char *txt_file, const char *bin_file)
{
    FILE *fp_txt, *fp_bin;
    int num;
    unsigned int length = 0;

    fp_txt = fopen(txt_file, "r");
    if (fp_txt == NULL)
    {
        printf("Error: open file %s failed\n", txt_file);
        return -1;
    }

    fp_bin = fopen(bin_file, "wb");
    if (fp_bin == NULL)
    {
        printf("Error: open file %s failed\n", bin_file);
        return -1;
    }

    while (fscanf(fp_txt, "%d", &num) != EOF)
    {
        //check if num is in range of 0~255
        if (num >= 0 && num < 256)
        {
            length += 1;
            //printf("0X%02X ", num);
            //write num to binary file
            fwrite(&num, sizeof(unsigned char), 1, fp_bin);
        }
        else
        {
            printf("Error: number %d is out of range\n", num);
            return -1;
        }
    }
    //printf("\n");
    fclose(fp_txt);
    fclose(fp_bin);
    return length;
}
/*Convert a binary file to text file seperated by space*/
int bin2txt(const char *binFile, const char *txtFile, unsigned int width, unsigned int inHex)
{
    FILE *fp_bin, *fp_txt;
    unsigned char num;
    unsigned int i;
    unsigned int length = 0;

    fp_bin = fopen(binFile, "rb");
    if (fp_bin == NULL)
    {
        printf("Error: open file %s failed\n", binFile);
        return -1;
    }

    fp_txt = fopen(txtFile, "w");
    if (fp_txt == NULL)
    {
        printf("Error: open file %s failed\n", txtFile);
        return -1;
    }

    while (fread(&num, sizeof(unsigned char), 1, fp_bin) != 0)
    {
        length++;
        if (inHex)
        {
            //printf("0X%02X ", num);
            fprintf(fp_txt, "0X%02X ", num);
        }
        else
        {
            //printf("%03d ", num);
            fprintf(fp_txt, "%03d ", num);
        }

        if (length % width == 0)
        {
            //printf("\n");
            fprintf(fp_txt, "\n");
        }
    }
    //printf("\n");
    fclose(fp_bin);
    fclose(fp_txt);
    return length;
}

char * clean_string(char *inStr, char *char_to_remove,int num)
// remove all characters in char_to_remove from inStr and store the result in outStr
//char_to_remove is the array of characters to be removed, num is the number of characters in char_to_remove
//return the pointer to the result string
{
    char *outStr;
    int i, j, k;
    //if num is 0 or char_to_remove is NULL, use predefined char_to_remove
    if (num == 0 || char_to_remove == NULL)
    {
        char_to_remove = "\t";
        num = 1;
    }

    //allocate memory for outStr
    int len = strlen(inStr);    
    outStr = (char *)malloc(len);

    for (i = 0, j = 0; i < len; i++)
    {
        for (k = 0; k < num; k++)
        {
            if (inStr[i] == char_to_remove[k])
            {
                break;
            }
        }
        if (k == num)
        {
            outStr[j++] = inStr[i];
        }
    }
    outStr[j] = '\0';
    return outStr;
}

char *single_pattern(char *inStr,char pattern)
//replace the continuous pattern with one pattern character
{
    char *outStr;
    int i, j;
    int len = strlen(inStr);
    outStr = (char *)malloc(len);

    for (i = 0, j = 0; i < len; i++)
    {
        if (inStr[i] != pattern)
        {
            outStr[j++] = inStr[i];
        }
        else
        {
            if (i == 0 || inStr[i - 1] != pattern)
            {
                outStr[j++] = inStr[i];
            }
        }
    }
    outStr[j] = '\0';
    return outStr;
}

//trim leading and tailing space from a string
char * line_trim(char *str)
{
    char *p = str;
    char *q = str;
    while (*p == ' ')
        p++;
    while (*p != '\0')
        *q++ = *p++;
    *q = '\0';

    //return the trimmed string
    return p; 
}

//Check if a number in the array, element is unsigned integer
unsigned int is_in_array(unsigned int *array, unsigned int length, unsigned int element)
{
    unsigned int i;
    for (i = 0; i < length; i++)
    {
        if (array[i] == element)
        {
            return 1;
        }
    }
    return 0;
}

//get the size of the file
unsigned int getFileSize(const char *fileName)
{
    FILE *fp;
    unsigned int size;
    fp = fopen(fileName, "rb");
    if (fp == NULL)
    {
        printf("Error: open file %s failed\n", fileName);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);
    return size;
}

//get number of integers in a file
int get_num_of_int(const char *fileName,int retLines)
{
    FILE *fp;
    char line[256];
    char *token;
    int count = 0;
    int lines = 0;
    unsigned char num;

    fp = fopen(fileName, "r");
    if (fp == NULL)
    {
        printf("Error: open file %s failed\n", fileName);
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        token = strtok(line, " ");
        while (token != NULL)
        {
            count++;
            token = strtok(NULL, " ");
        }
        lines++;        
    }
    fclose(fp);

    if(retLines) //if return lines, return lines
        return lines;
    else //if return number of integers, return number of integers
        return count;    
}


//Check how many columns in a text file , skip the empty lines
int get_columns_count(const char *txtFile)
{
    FILE *fp_txt;
    char line[256],*pline;
    char *token;
    int count = 0;       

    fp_txt = fopen(txtFile, "r");
    if (fp_txt == NULL)
    {
        printf("Error: open file %s failed\n", txtFile);
        return -1;
    }

    while (fgets(line, sizeof(line), fp_txt) != NULL)
    {   
        pline = &line[0];
        line_trim(pline);
        token = strtok(pline, " ");
        while (token != NULL)
        {            
            //check if the current token contains newline
            if (strchr(token, '\n') != NULL)
            {
                //if the token length is 1, just return count, otherwise return count+1
                if (strlen(token) == 1)
                    return count;
                else
                    return count+1;                
            }
            else
            {
                count++;
                token = strtok(NULL, " ");
            }            
        }

    }
    return count;    

}

/*Get the Nth column from a text file*/
int get_column(const char *txtFile, int column, const char *columnFile)
{
    FILE *fp_txt, *fp_column;
    char line[256];
    char *token;
    int count = 0;
    int index = 0;
    unsigned char num;

    fp_txt = fopen(txtFile, "r");
    if (fp_txt == NULL)
    {
        printf("Error: open file %s failed\n", txtFile);
        return -1;
    }

    fp_column = fopen(columnFile, "w");
    if (fp_column == NULL)
    {
        printf("Error: open file %s failed\n", columnFile);
        return -1;
    }

    while (fgets(line, sizeof(line), fp_txt) != NULL)
    {
        token = strtok(line, " ");
        while (token != NULL)
        {
            count++;
            if (count == column)
            {
                num = atoi(token);
                //fwrite(&token, sizeof(unsigned char), 1, fp_column);
                fprintf(fp_column, "0X%02X\n", num);
                index++;
            }
            token = strtok(NULL, " ");
        }
        count = 0;
    }
    fclose(fp_txt);
    fclose(fp_column);
    return index;
}

/*Get multiple columns from a text file*/
void get_columns(const char *txtFile, int total, int columnList[], const char *outFile)
{
    FILE *fp_txt, *fp_out;
    char line[256],*pline;
    char *token;
    int count = 0;
    int index = 0;
    int i;
    unsigned char num;   

    fp_txt = fopen(txtFile, "r");
    if (fp_txt == NULL)
    {
        printf("Error: open file %s failed\n", txtFile);
        return;
    }
        
    rewind(fp_txt);    
    fp_out = fopen(outFile, "w");
    if (fp_out == NULL)
    {
        printf("Error: open file %s failed\n", outFile);
        return;
    }

    while (fgets(line, sizeof(line), fp_txt) != NULL)
    {
        pline = &line[0];                     
        pline = clean_string(pline,NULL,0); //remove the '\t'
        pline = single_pattern(pline,' ');  //remove duplicate space
        pline = single_pattern(pline,'\n'); //remove dulicate newline

        line_trim(pline);                          
     
        token = strtok(pline, " ");
        while (token != NULL )
        {
            count++;           
            //check if the current token contains newline
            if (strchr(token, '\n') != NULL)
            {
                if(count>0)
                {
                    fprintf(fp_out, "\n");
                    count = 0 ;
                    break;
                }
                else //This is empty line
                    break;
                
            }
            else if(is_in_array(columnList, total, count))  //This column is in the list, should be kept
            {            
                num = atoi(token);
                fprintf(fp_out, "%03d ", num);
            }

            token = strtok(NULL, " ");

        }
        count = 0;
    }
    fclose(fp_txt);
    fclose(fp_out);
}


/*Delete the some columns from a text file*/
void delete_columns(const char *txtFile, int total, int columnList[], const char *outFile)
{
    FILE *fp_txt, *fp_out;
    char line[256],*pline;
    char *token;
    int count = 0;
    int index = 0;
    int i;
    unsigned char num;   

    fp_txt = fopen(txtFile, "r");
    if (fp_txt == NULL)
    {
        printf("Error: open file %s failed\n", txtFile);
        return;
    }
        
    rewind(fp_txt);    
    fp_out = fopen(outFile, "w");
    if (fp_out == NULL)
    {
        printf("Error: open file %s failed\n", outFile);
        return;
    }

    while (fgets(line, sizeof(line), fp_txt) != NULL)
    {
        pline = &line[0];        
        line_trim(pline);
        token = strtok(pline, " ");
        while (token != NULL)
        {
            count++;           
            //check if the current token contains newline
            if (strchr(token, '\n') != NULL)
            {
                if(count>0)
                {
                    fprintf(fp_out, "\n");
                    count = 0 ;
                    break;
                }
                else //This is empty line
                    break;
                
            }
            else if(!is_in_array(columnList, total, count))  //This column is not in the list, should be kept
            {
                num = atoi(token);
                fprintf(fp_out, "%03d ", num);
            }

            token = strtok(NULL, " ");

        }
        count = 0;
    }
    fclose(fp_txt);
    fclose(fp_out);
}

void dumpTextFile(const char *txtFile)
{
    FILE *fp_txt;
    char line[256];

    fp_txt = fopen(txtFile, "r");
    if (fp_txt == NULL)
    {
        printf("Error: open file %s failed\n", txtFile);
        return;        
    }

    //print filename as title
    printf("\n%s\n", txtFile);

    while (fgets(line, sizeof(line), fp_txt) != NULL)
    {
        printf("%s", line);
    }
    fclose(fp_txt);
}

//dump a binary file in hex format, with a specified number of bytes per line
void dumpBinaryFile(const char *fileName, int bytesPerLine, int linesPerPage)
{
    FILE *fp;
    unsigned char buffer[bytesPerLine];
    unsigned int i, j;
    unsigned int totalBytes;
    unsigned int totalLines;
    unsigned int totalPages;
    unsigned int page;
    unsigned int line;
    unsigned int offset;
    unsigned int bytes;

    fp = fopen(fileName, "rb");
    if (fp == NULL)
    {
        printf("Error: open file %s failed\n", fileName);
        return;
    }

    totalBytes = getFileSize(fileName);
    totalLines = totalBytes / bytesPerLine;
    totalPages = totalLines / linesPerPage;

    //print absolute filename as title    
    printf("\n%s\n", fileName);    
    for (page = 0; page < totalPages; page++)
    {
        printf("\nPage %d\n", page);
        //add a header line for column index               
        for (line = 0; line < linesPerPage; line++)
        {
            offset = page * linesPerPage * bytesPerLine + line * bytesPerLine;
            fseek(fp, offset, SEEK_SET);
            bytes = fread(buffer, 1, bytesPerLine, fp);
            if (bytes != bytesPerLine)
            {
                printf("Error: read file %s failed\n", fileName);
                return;
            }
            printf("%08X: ", offset);
            for (i = 0; i < bytesPerLine; i++)
            {
                printf("%02X ", buffer[i]);
            }
            printf(" ||  ");
            for (i = 0; i < bytesPerLine; i++)
            {
             printf("%03d ", buffer[i]);
            }
            printf("\n");
        }
    }

    fclose(fp);
}

//popup an open file dialog and return the file name
char *openFileDialog(const char *title, const char *filter)
{
    char *fileName = NULL;
    OPENFILENAME ofn;
    char szFileName[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "txt";

    if (GetOpenFileName(&ofn))
    {
        fileName = (char *)malloc(strlen(szFileName) + 1);
        strcpy(fileName, szFileName);
    }
    return fileName;
}

//get the absolute file name from a full path
char *get_file_name(const char *fullPath)
{
    char *fileName = NULL;
    char *p = NULL;
    int len = 0;

    fileName = (char *)malloc(strlen(fullPath) + 1);
    strcpy(fileName, fullPath);
    p = strrchr(fileName, '\\');
    if (p != NULL)
    {
        len = strlen(p);
        fileName[strlen(fileName) - len] = '\0';
    }
    return fileName;
}

//get the absolute filename from fullpath without extension
char *get_file_name_without_ext(const char *fullPath)
{
    char *fileName = NULL;
    char *p = NULL;
    int len = 0;

    fileName = (char *)malloc(strlen(fullPath) + 1);
    strcpy(fileName, fullPath);
    p = strrchr(fileName, '\\');
    if (p != NULL)
    {
        len = strlen(p);
        strcpy(fileName, p+1); //skip the '\\'
        fileName[len+1] = '\0';
    }
    p = strrchr(fileName, '.');
    if (p != NULL)
    {
        len = strlen(p);
        fileName[strlen(fileName) - len] = '\0';
    }
    return fileName;
}

//get the extension of a file
char *get_file_ext(const char *fullPath)
{
    char *p = NULL;    
    p = strrchr(fullPath, '.');
    return p;
}

//get the directory of a file
char *get_file_dir(const char *fullPath)
{
    char *fileDir = NULL;
    char *p = NULL;
    int len = 0;

    fileDir = (char *)malloc(strlen(fullPath) + 1);
    strcpy(fileDir, fullPath);
    p = strrchr(fileDir, '\\');
    if (p != NULL)
    {
        len = strlen(p)-1;
        fileDir[strlen(fileDir) - len] = '\0';
    }
    return fileDir;
}
//replace the extension of a file
char *replace_file_ext(const char *fullPath, const char *newExt)
{
    char *filePath = NULL;
    char *p = NULL;
    int len = 0;

    filePath = (char *)malloc(strlen(fullPath) + 1);
    strcpy(filePath, fullPath);
    p = strrchr(filePath, '.');
    if (p != NULL)
    {
        len = strlen(p);
        filePath[strlen(filePath) - len] = '\0';
    }
    strcat(filePath, newExt);
    return filePath;
}

//make a new filename from directory and file name and extension
char *make_file_path(const char *dir, const char *fileName, const char *ext)
{
    char *filePath = NULL;
    int len = 0;
    
    filePath = (char *)malloc(strlen(dir) + strlen(fileName) + strlen(ext) + 1);
    strcpy(filePath, dir);
    strcat(filePath, fileName);
    strcat(filePath, ext);
    return filePath;
}

//add prefix to a string
char *str_add_prefix(const char *str, const char *prefix)
{
    char *newStr = NULL;
    int len = 0;

    newStr = (char *)malloc(strlen(str) + strlen(prefix) + 1);
    strcpy(newStr, prefix);
    strcat(newStr, str);
    return newStr;
}

//add suffix to a string
char *str_add_suffix(const char *str, const char *suffix)
{
    char *newStr = NULL;
    int len = 0;

    newStr = (char *)malloc(strlen(str) + strlen(suffix) + 1);
    strcpy(newStr, str);
    strcat(newStr, suffix);

    return newStr;
}

//replace filename in a full path
char *replace_filename(const char *fullPath, const char *newFileName)
{
    char *filePath = NULL;
    char *p = NULL;
    int len = 0;

    char *filedir = get_file_dir(fullPath);
    char *oldfilename =get_file_name_without_ext(fullPath);
    char *oldext = get_file_ext(fullPath);    
    char *newfullPath = make_file_path(filedir, newFileName, oldext);
    
    free(filedir);
    free(oldfilename);    
    
    return newfullPath;  

}

//add prefix to the absolute filename and return the new filepath
char *add_prefix_to_filename(const char *fullPath, const char *prefix)
{
    char *filePath = NULL;
    char *p = NULL;
    int len = 0;

    char *filedir = get_file_dir(fullPath);
    char *oldfilename = get_file_name_without_ext(fullPath);
    char *oldext = get_file_ext(fullPath);
    char *newfilename = str_add_prefix(oldfilename, prefix);
    char *newfullPath = make_file_path(filedir, newfilename, oldext);
    
    free(filedir);
    free(oldfilename);    
    free(newfilename);

    return newfullPath;
}

//add surfix to the absolute filename and return the new filepath
char *add_suffix_to_filename(const char *fullPath, const char *suffix)
{
    char *filePath = NULL;
    char *p = NULL;
    int len = 0;

    char *filedir = get_file_dir(fullPath);    
    //printf("filedir: %s\n", filedir);        
    char *oldfilename = get_file_name_without_ext(fullPath);    
    //printf("oldfilename: %s\n", oldfilename);
    char *oldext = get_file_ext(fullPath);   
    //printf("oldext: %s\n", oldext); 
    char *newfilename = str_add_suffix(oldfilename, suffix);        
    //printf("newfilename: %s\n", newfilename);    
    char *newfullPath = make_file_path(filedir, newfilename, oldext);
    //printf("newfullPath: %s\n", newfullPath);
            
    free(filedir);
    free(oldfilename);    
    free(newfilename);
    
    return newfullPath;
}

//check if a file exists, if exist delete it else do nothing
int delete_file(const char *fileName)
{
    FILE *fp;
    if ((fp = fopen(fileName, "r")) == NULL)
    {
        return 0;
    }
    else
    {
        fclose(fp);
        remove(fileName);
        return 1;
    }
}

#ifndef _WIN32
int main(void)
{
    int width = 9;
    int columnsList[3]={1, 3, 5};

    txt2bin("numbers.txt", "numbers.bin");
    bin2txt("numbers.bin", "numbers_hex.txt", width, 1);
    bin2txt("numbers.bin", "numbers_dec.txt", width, 0);    
    get_column("numbers_hex.txt", 2, "numbers_column2.txt");    

    dumpTextFile("numbers_dec.txt");    
    delete_columns("numbers_dec.txt", 3, columnsList, "numbers_afterDelete.txt");
    dumpTextFile("numbers_afterDelete.txt");    

    int size  = getFileSize("numbers_afterDelete.txt");
    int count = get_num_of_int("numbers_afterDelete.txt",0);
    int lines = get_num_of_int("numbers_afterDelete.txt",1);
    int columns = get_columns_count("numbers_afterDelete.txt");

    printf("file=%s, size = %d bytes, lines=%d, columns= %d, number_of_int = %d \n","numbers_afterDelete.txt", size, lines, columns, count);
    return 0;
}
#else
//Standard windows main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int columnsList[3]={1, 3, 5};
    const char *srcFile = openFileDialog("Open source file", "Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0");
    //printf("srcFile=%s\n", srcFile);

    int width = get_columns_count(srcFile);
    //printf("width=%d\n", width);

    const char *dstFile = replace_file_ext(srcFile, ".bin");
    //printf("dstFile=%s\n", dstFile);

    const char *dstFile_hex = add_suffix_to_filename(srcFile, "_hex");
    //printf("dstFile_hex=%s\n", dstFile_hex);

    const char *dstFile_dec = add_suffix_to_filename(srcFile, "_dec");
   // printf("dstFile_dec=%s\n", dstFile_dec);

    const char *dstFile_column2 = add_suffix_to_filename(srcFile, "_column2");
    //printf("dstFile_column2=%s\n", dstFile_column2);

    const char *dstFile_column1_3_5 = add_suffix_to_filename(srcFile, "_column1_3_5");

    const char *dstFile_afterDelete = add_suffix_to_filename(srcFile, "_afterDelete");
    //printf("dstFile_afterDelete=%s\n", dstFile_afterDelete);
        
    free((void *)srcFile);
    free((void *)dstFile);    
    free((void *)dstFile_hex);

    dumpTextFile(srcFile);  
    get_columns(srcFile, 3,columnsList, dstFile_column1_3_5);
    dumpTextFile(dstFile_column1_3_5);  
     
    dumpTextFile(srcFile);  
    txt2bin(srcFile, dstFile);    

    dumpBinaryFile(dstFile, width,10);

    bin2txt(dstFile, dstFile_hex, width, 1);
    dumpTextFile(dstFile_hex);

    bin2txt(dstFile, dstFile_dec, width, 0);    
    dumpTextFile(dstFile_dec);
    
    get_column(dstFile_hex, 2, dstFile_column2);
    dumpTextFile(dstFile_column2);

    dumpTextFile(dstFile_dec);   
    delete_columns(dstFile_dec, 3, columnsList, dstFile_afterDelete);
    dumpTextFile(dstFile_afterDelete);

    int size  = getFileSize(dstFile_afterDelete);
    int count = get_num_of_int(dstFile_afterDelete,0);
    int lines = get_num_of_int(dstFile_afterDelete,1);
    int columns = get_columns_count(dstFile_afterDelete);

    printf("\nfile=%s, size = %d bytes, lines=%d, columns= %d, number_of_int = %d \n",get_file_name(dstFile_afterDelete), size, lines, columns, count);

    return 0;
}
#endif


