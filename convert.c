#include "stdio.h"
#include "stdlib.h"
#include "string.h"

//#include <Commdlg.h>
#include <windows.h>

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

int bin2Frame(const char *binFile, const char *FrameFile, unsigned int FrameLength,
              unsigned char *openFlag, unsigned int flagLen, unsigned int *compareMask, unsigned int checkerCount)
{
    FILE *fp_bin, *fp_frame;

    unsigned long totalFrame, actualFrame, uniqueFrame, fSize, fRead, fLeft, fFrame;
    unsigned long i, j;
    unsigned char *pFlag;
    unsigned char *previous_frame;
    unsigned char *next_frame;
    unsigned char *previous_frame_in_mask;
    unsigned char *next_frame_in_mask;
    unsigned int flagDetected, Same;

    //validate input
    if (FrameLength == 0 || flagLen == 0)
    {
        printf("Error: FrameLength or flagLen is 0\n");
        return -1;
    }

    fp_bin = fopen(binFile, "rb");
    if (fp_bin == NULL)
    {
        printf("Error: open file %s failed\n", binFile);
        return -1;
    }

    fp_frame = fopen(FrameFile, "w");
    if (fp_frame == NULL)
    {
        printf("Error: open file %s failed\n", FrameFile);
        return -1;
    }

    previous_frame = (unsigned char *)malloc(FrameLength);
    next_frame = (unsigned char *)malloc(FrameLength);
    pFlag = (unsigned char *)malloc(flagLen);
    //memset pFlag and pFrame
    memset(pFlag, 0, flagLen);
    memset(previous_frame, 0, FrameLength);
    memset(next_frame, 0, FrameLength);

    if (previous_frame == NULL || next_frame == NULL || pFlag == NULL)
    {
        //prompt error detail and exit

        printf("Error: allocate memory for previous_frame or next_frame or pFlag failed\n");
        return -1;
    }

    if (checkerCount) //there are checker bytes, allocate memory for previous_frame_in_mask and next_frame_in_mask
    {
        previous_frame_in_mask = (unsigned char *)malloc(checkerCount);
        next_frame_in_mask = (unsigned char *)malloc(checkerCount);
        memset(previous_frame_in_mask, 0, checkerCount);
        memset(next_frame_in_mask, 0, checkerCount);
        //validate the allocated memory
        if (previous_frame_in_mask == NULL || next_frame_in_mask == NULL)
        {
            printf("Error: allocate memory for previous_frame_in_mask or next_frame_in_mask failed\n");
            return -1;
        }
    }

    fseek(fp_bin, 0, SEEK_END);
    fSize = ftell(fp_bin);
    totalFrame = (unsigned long)(fSize / FrameLength);
    printf("fSize = %ld\n", fSize);
    printf("totalFrame = %ld\n", totalFrame);
    //print input parameters
    printf("FrameLength = %d\n", FrameLength);
    printf("flagLen = %d\n", flagLen);
    //print flag
    printf("open flag = ");
    for (i = 0; i < flagLen; i++)
    {
        printf("%02X ", openFlag[i]);
    }
    printf("\n");
    //print compareMask
    printf("checkerCount = %d\n", checkerCount);
    printf("compareMask = ");
    for (i = 0; i < checkerCount; i++)
    {
        printf("%d ", compareMask[i]);
    }
    printf("\n");       

    if (totalFrame == 0 || fSize <= FrameLength)
    {
        printf("Error: totalFrame is 0 or fSize <= FrameLength\n");
        return -1;
    }

    actualFrame = 0;
    uniqueFrame = 0;
    fRead = 0;
    fLeft = fSize;
    fFrame = 0;
    flagDetected = 0;
    Same = 0;

    //start from the head of the file
    fseek(fp_bin, 0, SEEK_SET);

    //Every search starting from locating the header of the frame, the is the outer loop.

    while (fLeft >= FrameLength)
    {
        //locate the first openflag from the beginning of the pbin file
        while (fLeft >= flagLen)
        {
            int bytesRead = fread(pFlag, sizeof(unsigned char), flagLen, fp_bin);
            if (memcmp(pFlag, openFlag, flagLen) == 0)
            {
                printf("openflag found [fRead = %d  fLeft=%d]\n",fRead,fLeft);
                flagDetected = 1;
                fRead += flagLen;
                fLeft -= flagLen;
                break;
            }
            else
            {
                //printf("openflag not found\n");
                fseek(fp_bin, -(flagLen - 1), SEEK_CUR);
                fRead +=1;
                fLeft -=1;
            }
        }
        //check if we detected the flag, or the fLeft is less than flagLen
        if (!flagDetected || fLeft < (FrameLength - flagLen))
        {
            //check if we have already get any frames written to FrameFile, if so, break the loop
            if (actualFrame)
            {
                break;
            }
            else
            {
                printf("Error: openflag not found\n");
                //close files if they're open
                if (fp_bin)
                {
                    fclose(fp_bin);
                }
                if (fp_frame)
                {
                    fclose(fp_frame);
                }
                //free memory if they're allocated
                if (previous_frame)
                {
                    free(previous_frame);
                }
                if (next_frame)
                {
                    free(next_frame);
                }
                if (pFlag)
                {
                    free(pFlag);
                }
                if (previous_frame_in_mask)
                {
                    free(previous_frame_in_mask);
                }
                if (next_frame_in_mask)
                {
                    free(next_frame_in_mask);
                }
                return -1;
            }
        }
        else
        { //flag detected, we need to read (FrameLength-flagLen) bytes to the currentFrame

            //first move the nextFrame to previousFrame
            memcpy(previous_frame, next_frame, FrameLength);

            //also move the nextFrame_in_mask to previousFrame_in_mask
            if (checkerCount)
            {
                memcpy(previous_frame_in_mask, next_frame_in_mask, checkerCount);
            }

            //then fill the flag bytes into the nextFrame
            memcpy(next_frame, pFlag, flagLen);
            //then read the rest of the frame from the bin file
            int bytesRead = fread(next_frame + flagLen, sizeof(unsigned char), FrameLength - flagLen, fp_bin);

            //check if we have read enough bytes
            if (bytesRead < (FrameLength - flagLen))
            {
                if(actualFrame)
                {
                    break;
                }
                else //no actual frame
                {
                printf("Error: read frame failed\n");
                //close files if they're open
                if (fp_bin)
                {
                    fclose(fp_bin);
                }
                if (fp_frame)
                {
                    fclose(fp_frame);
                }
                //free memory if they're allocated
                if (previous_frame)
                {
                    free(previous_frame);
                }
                if (next_frame)
                {
                    free(next_frame);
                }
                if (pFlag)
                {
                    free(pFlag);
                }
                if (previous_frame_in_mask)
                {
                    free(previous_frame_in_mask);
                }
                if (next_frame_in_mask)
                {
                    free(next_frame_in_mask);
                }
                return -1;
               }//no actual frame
            }
            else  //we got enough bytes in this frame
            {

            fRead += (FrameLength - flagLen);
            fLeft -= (FrameLength - flagLen);

            actualFrame++; //everytime when we get a whole frame, we increase the actualFrame by 1

            //check if the frame is the same as the previous frame
            if (checkerCount)
            {
                //mask the checker bytes
                for (j = 0; j < checkerCount; j++)
                    next_frame_in_mask[j] = next_frame[compareMask[j] - 1]; //compareMask[j] is the jth checker byte index, index counting from 1

                //check if the checker bytes are the same
                if (memcmp(previous_frame_in_mask, next_frame_in_mask, checkerCount) == 0)
                {
                    Same = 1;
                }
                else
                {
                    Same = 0;
                }
            }
            else //there is no checker bytes, by default we don't run the check, all frames are unique and they should be written to FrameFile
            {
                Same = 0;
            }

            //if the Same is true, we need to skip the frame
            if (Same)
            {
                //printf("Same frame\n");
                continue;
            }
            else
            {
                //convert the chars in next_frame to number in hex format, and write to the frame file
                for (i = 0; i < FrameLength; i++)
                {
                    fprintf(fp_frame, "%02X ", next_frame[i]);
                }
                fprintf(fp_frame, "\n");
                uniqueFrame++;
            }
            //continue the loop for the next openFlag and frame
          } //we got enough bytes in this frame

        }//flag detected
    }     //while(fLeft >=FrameLength) outer loop

    //close files
    fclose(fp_bin);
    fclose(fp_frame);

    //free memories
    free(previous_frame);
    free(next_frame);
    free(pFlag);
    if (checkerCount)
    {
        free(previous_frame_in_mask);
        free(next_frame_in_mask);
    }

    //Now we print the summary of all the informations including file path and size, total frame, unique frame, and the frame file path
    printf("\n");
    printf(" Bin Filename : %s\n", binFile);
    printf("Bin File size : %ld bytes\n", fSize);
    printf(" Total frames : %ld\n", totalFrame);
    printf("Actual frames : %ld\n", actualFrame);
    printf("Unique frames : %ld\n", uniqueFrame);
    printf("Dropped frames: %ld\n", actualFrame - uniqueFrame);
    printf("   Read Bytes : %ld\n", fRead);
    printf("   Left Bytes : %ld\n", fLeft);
    printf("\n");
    printf("Frame Filename: %s\n", FrameFile);
    printf("\n");

    //print the framelength, openFlag length  and checkercount value
    printf("Config Data\n");
    printf("Frame length  : %d\n", FrameLength);
    printf("OpenFlag len  : %d\n", flagLen);
    printf("Open flags    : ");
    for (i = 0; i < flagLen; i++)
    {
        printf("%02X ", openFlag[i]);
    }
    printf("\n");

    printf("Checker count : %d\n", checkerCount);

    //also print the checker bytes, openFlag and compareMask if there are any
    if (checkerCount)
    {
        printf("Checker bytes : ");
        for (i = 0; i < checkerCount; i++)
        {
            printf("%02d ", compareMask[i]);
        }
        printf("\n");

        printf("\n");
    }
    //return
    return uniqueFrame;
}

char *clean_string(char *inStr, char *char_to_remove, int num)
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

char *single_pattern(char *inStr, char pattern)
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
char *line_trim(char *str)
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
unsigned int get_num_of_hex(char *str)
{
    char *token;
    int count = 0;
    token = strtok(str, " ");
    while (token != NULL)
    {
        //check if the token is a hex number
        if (strlen(token) == 4 && token[0] == '0' && (token[1] == 'X' || token[1] == 'x'))
        {
            count++;
        }
        count++;
        token = strtok(NULL, " ");
    }
    return count;
}

//get number of integers in a file
int get_num_of_int(const char *fileName, unsigned int retLines)
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

    if (retLines) //if return lines, return lines
        return lines;
    else //if return number of integers, return number of integers
        return count;
}

//Check how many columns in a text file , skip the empty lines
int get_columns_count(const char *txtFile)
{
    FILE *fp_txt;
    char line[256], *pline;
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
                    return count + 1;
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
    char line[256], *pline;
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
        pline = clean_string(pline, NULL, 0); //remove the '\t'
        pline = single_pattern(pline, ' ');   //remove duplicate space
        pline = single_pattern(pline, '\n');  //remove dulicate newline

        line_trim(pline);

        token = strtok(pline, " ");
        while (token != NULL)
        {
            count++;
            //check if the current token contains newline
            if (strchr(token, '\n') != NULL)
            {
                if (count > 0)
                {
                    fprintf(fp_out, "\n");
                    count = 0;
                    break;
                }
                else //This is empty line
                    break;
            }
            else if (is_in_array(columnList, total, count)) //This column is in the list, should be kept
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
    char line[256], *pline;
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
                if (count > 0)
                {
                    fprintf(fp_out, "\n");
                    count = 0;
                    break;
                }
                else //This is empty line
                    break;
            }
            else if (!is_in_array(columnList, total, count)) //This column is not in the list, should be kept
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
            printf("%010d: ", offset);
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
        strcpy(fileName, p + 1); //skip the '\\'
        fileName[len + 1] = '\0';
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
        len = strlen(p) - 1;
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
    char *oldfilename = get_file_name_without_ext(fullPath);
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

int testCase_Console()
{

    int width = 9;
    int columnsList[3] = {1, 3, 5};

    txt2bin("numbers.txt", "numbers.bin");
    bin2txt("numbers.bin", "numbers_hex.txt", width, 1);
    bin2txt("numbers.bin", "numbers_dec.txt", width, 0);
    get_column("numbers_hex.txt", 2, "numbers_column2.txt");

    dumpTextFile("numbers_dec.txt");
    delete_columns("numbers_dec.txt", 3, columnsList, "numbers_afterDelete.txt");
    dumpTextFile("numbers_afterDelete.txt");

    int size = getFileSize("numbers_afterDelete.txt");
    int count = get_num_of_int("numbers_afterDelete.txt", 0);
    int lines = get_num_of_int("numbers_afterDelete.txt", 1);
    int columns = get_columns_count("numbers_afterDelete.txt");

    printf("file=%s, size = %d bytes, lines=%d, columns= %d, number_of_int = %d \n", "numbers_afterDelete.txt", size, lines, columns, count);
    return 0;
}

int testCase_GUI()
{
    int columnsList[3] = {1, 3, 5};
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
    get_columns(srcFile, 3, columnsList, dstFile_column1_3_5);
    dumpTextFile(dstFile_column1_3_5);

    dumpTextFile(srcFile);
    txt2bin(srcFile, dstFile);

    dumpBinaryFile(dstFile, width, 10);

    bin2txt(dstFile, dstFile_hex, width, 1);
    dumpTextFile(dstFile_hex);

    bin2txt(dstFile, dstFile_dec, width, 0);
    dumpTextFile(dstFile_dec);

    get_column(dstFile_hex, 2, dstFile_column2);
    dumpTextFile(dstFile_column2);

    dumpTextFile(dstFile_dec);
    delete_columns(dstFile_dec, 3, columnsList, dstFile_afterDelete);
    dumpTextFile(dstFile_afterDelete);

    int size = getFileSize(dstFile_afterDelete);
    int count = get_num_of_int(dstFile_afterDelete, 0);
    int lines = get_num_of_int(dstFile_afterDelete, 1);
    int columns = get_columns_count(dstFile_afterDelete);

    printf("\nfile=%s, size = %d bytes, lines=%d, columns= %d, number_of_int = %d \n", get_file_name(dstFile_afterDelete), size, lines, columns, count);

    return 0;
}

#ifndef _WIN32
int main(void)
{
    retun 1;
}
#else
//Standard windows main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    //pop up open file dialog to choose a binary file (*.bin or *.dat) for function bin2Frame
    const char *srcFile = openFileDialog("Open source file", "Binary files (*.bin;*.dat)\0*.bin;*.dat\0All files (*.*)\0*.*\0");
    printf("srcFile=%s\n", srcFile);

    //generate a new file name with .txt extension
    const char *dstFile = replace_file_ext(srcFile, ".txt");
    printf("dstFile=%s\n", dstFile);

    //add a suffix _frame to the file name
    const char *dstFile_frame = add_suffix_to_filename(dstFile, "_frame");
    printf("dstFile_frame=%s\n", dstFile_frame);

    //now free the memory
    //free((void *)srcFile);
    free((void *)dstFile);

    unsigned int frameLen = 16;
    unsigned int flagLen = 2;
    unsigned char flag[2] = {0xEB, 0x90};
    unsigned int frameCount = 0;

    //convert the binary file to a frame file
    frameCount = bin2Frame(srcFile, dstFile_frame, frameLen, flag, flagLen, NULL, 0);

    //dump the srcFile to the console
    //dumpBinaryFile(srcFile, frameLen, 32);

    //dump the dstFile_frame to the console
   // dumpTextFile(dstFile_frame);
}
#endif
