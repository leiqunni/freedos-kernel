/*****************************************************************************
** PATCHOBJ facility - change strings in the LNAMES record					 
**
** Copyright 2001 by tom ehlert
**
** GPL bla to be added, but intended as GPL
**       
**
** 09/06/2001 - initial revision
** not my biggest kind of software; anyone willing to add 
** comments, errormessages, usage info,...???
** 
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef TRUE 
	#define TRUE (1==1)
	#define FALSE (0==1)
#endif	


struct {
	char *sin,*sout;
	} repl[100];
int repl_count;
    

void quit(char *s,...)
{
    vprintf(s,(void*)((char *)&s+sizeof(s)));
    exit(1);
}

void define_replace(char *sin)
{
	char *s;
	
	if (repl_count >= 99)
		quit("too many replacements");
     
	if ((s = strchr(sin, '=')) == NULL)
		quit("illegal replacement <%s>, missing '='", sin);

	*s = 0;
	repl[repl_count].sin  = sin;		
	repl[repl_count].sout = s+1;		
	
	repl_count++;		
}

int main(int argc, char *argv[])
{
	char *argptr;
	int argloop;
	int cont;
	int file = 0;
	FILE *fd,*fdo;
	char *inname=0,*outname= "~patchob.tmp";

	int use_temp_file = TRUE;

	argc--,argv++;

	for (; argc != 0; argc--,argv++)
		{
		argptr = *argv;
					
		if (*argptr != '-' && *argptr != '-')
			{
			if (inname == 0) { inname = argptr; continue;}
			define_replace(argptr);
			continue;
			}        
		switch (toupper(argptr[1]))
			{
			case 'O': 
				outname = argptr+2; 
				use_temp_file = FALSE;
				break;
			default:
				quit("illegal argument <%s>\n",argptr);				
				break;
			}
		}

	

	if (inname == 0)
		quit("Inputfile must be specified\n");

	if (repl_count == 0)
		quit("no replacements defined");


	if ((fd = fopen(inname,"rb")) == NULL)		/* open for READ/WRITE		*/
		quit("can't read %s\n",inname);

	if ((fdo = fopen(outname,"wb")) == NULL)		/* open for READ/WRITE		*/
		quit("can't write %s\n",outname);

	go_records(fd,fdo);

	fclose(fd);
	fclose(fdo);
	if (use_temp_file)
		{
		unlink(inname);
		rename(outname,inname);
		}

	return 0;

}

usage()
{
	fputs(
		"DELSYM V1.0 5'95by tom ehlert, SIG Aachen\n"
		"       delete symbolic info in object files\n"
		"usage:\n"
		"     DELSYM infile [outfile]\n"
		,stderr);
	exit(1);
}

struct record
	{
	unsigned char rectyp;
	unsigned datalen;
	unsigned char buffer[0x2000];
	} Record,Outrecord;


go_records(FILE *fdin,FILE *fdo)
{            
	unsigned char stringlen;
	unsigned char *string,*s;                              
	int i,j;
	unsigned char chksum;

do  {
	if (fread(&Record,1,3,fdin) != 3)  {	/* read type and reclen*/
		/* printf("end of fdin read\n"); */
		break;
		}
	if (Record.datalen > sizeof(Record.buffer))
		quit ("record to large : length %u Bytes \n",Record.datalen);

	if (fread(Record.buffer,1,Record.datalen,fdin) != Record.datalen)
		{
		printf("invalid record format\n");
		quit ("can't continue\n");
		}

	
	
	if (Record.rectyp != 0x96)	/* we are only interested in LNAMES */
		{
		fwrite(&Record,1,3+Record.datalen,fdo);
		continue;
		}

	Outrecord.rectyp = 0x96;
	Outrecord.datalen= 0;


			
			

	
	for (i = 0; i < Record.datalen-1; )
		{
		stringlen = Record.buffer[i];
		i++;
		
		string = &Record.buffer[i];
		i += stringlen;
		
		
		if (i > Record.datalen)
			quit("invalid lnames record");
	
		
		for (j = 0; j < repl_count; j++)
			if (memcmp(string,repl[j].sin,stringlen) == 0
				&& strlen(repl[j].sin) == stringlen)
				{
				string    = repl[j].sout;
				stringlen = strlen(repl[j].sout);
				}
		Outrecord.buffer[Outrecord.datalen] = stringlen;
		Outrecord.datalen++;
		memcpy(Outrecord.buffer+Outrecord.datalen,string,stringlen);
		Outrecord.datalen += stringlen;
		}		

	chksum = 0;
	for (s = (unsigned char *)&Outrecord; s < &Outrecord.buffer[Outrecord.datalen]; s++)
		chksum += *s;
	
	Outrecord.buffer[Outrecord.datalen] = ~chksum;	
	Outrecord.datalen++;
		
	/* printf("^sum = %02x - %02x\n",chksum,~chksum); */
	
	
	fwrite(&Outrecord,1,3+Outrecord.datalen,fdo);
	
	} while (Record.rectyp != 0x00 /*ENDFIL*/);

printf("\n");
}
