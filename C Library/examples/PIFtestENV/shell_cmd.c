
#include "shell_cmd.h"

extern microcontrollerBoard_t *mcuBoard;

static const _GMEMX char * const _GMEMX fatfs_rc_str[] = 
{
	G_XARR("OK"),
	G_XARR("DISK_ERR"),
	G_XARR("INT_ERR"),
	G_XARR("NOT_READY"),
	G_XARR("NO_FILE"),
	G_XARR("NO_PATH"),
	G_XARR("INVALID_NAME"),
	G_XARR("DENIED"),
	G_XARR("EXIST"),
	G_XARR("INVALID_OBJECT"),
	G_XARR("WRITE_PROTECTED"),
	G_XARR("INVALID_DRIVE"),
	G_XARR("NOT_ENABLED"),
	G_XARR("NO_FILESYSTEM"),
	G_XARR("MKFS_ABORTED"),
	G_XARR("TIMEOUT"),
	G_XARR("LOCKED"),
	G_XARR("NOT_ENOUGH_CORE"),
	G_XARR("TOO_MANY_OPEN_FILES"),
	G_XARR("INVALID_PARAMETER")
};

static const _GMEMX char * const _GMEMX fatfs_type[] = 
{
	G_XARR(""),
	G_XARR("FAT12"),
	G_XARR("FAT16"),
	G_XARR("FAT32"),
	G_XARR("exFAT")
};

static void ret_fatfs_result(FRESULT rc)
{
	gshell_putString_flash(fatfs_rc_str[rc]);
}

static void dump_line(const uint8_t *buff, uint32_t ofs, uint8_t cnt)
{
	uint8_t i;
	
	gshell_printf_f("%08X:", ofs);
	
	for (i = 0; i < cnt; i++)
	{
		gshell_printf_f(" %02X", buff[i]);
	}
	
	if (cnt != 16)
	{
		for (i = cnt; i < 16; i++)
		{
			gshell_putString_f("   ");
		}
	}
	
	gshell_putString_f("  "G_TEXTREVERSE);
	
	for (i = 0; i < cnt; i++)
	{
		gshell_putChar((buff[i] >= ' ' && buff[i] <= '~') ? buff[i] : '.');
	}
	
	gshell_putString(G_TEXTNORMAL"\r\n");
}

FIL File[2];
DIR Dir;
FILINFO Finfo;
static FRESULT scan_files (
	char* path,		/* Pointer to the working buffer with start path */
	UINT* n_dir, UINT* n_file, DWORD* sz_file)
{
	DIR dirs;
	FRESULT fr;
	int i;

	fr = f_opendir(&dirs, path);
	if (fr == FR_OK) {
		while (((fr = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0]) {
			if (Finfo.fattrib & AM_DIR) {
				(*n_dir)++;
				i = strlen(path);
				path[i] = '/'; strcpy(path+i+1, Finfo.fname);
				fr = scan_files(path, n_dir, n_file, sz_file);
				path[i] = 0;
				if (fr != FR_OK) break;
				} else {
				//				xprintf(PSTR("%s/%s\n"), path, Finfo.fname);
				(*n_file)++;
				*sz_file += Finfo.fsize;
			}
		}
	}

	return fr;
}

static uint8_t workBuf[FF_MAX_SS];
static void cli_cmd_fatfs(uint8_t argc, char *argv[])
{
	uint16_t drv = 0, acc_dirs, acc_files,dcheck;
	static uint32_t sec = 0;
	uint32_t dval, dtemp;
	FRESULT fr;
	uint8_t format_unlocked = 1, u8temp;
	char line[32];
	FATFS *fptr_cli = &(mcuBoard->fs);
	
	if (argc == 1)
	{
		gshell_putString_f("Type 'fatfs ?' to show the commands\r\n");
		return;
	}
	
	switch(argv[1][0])
	{
		case 'd':
			switch (argv[1][1])
			{
				case 'd':	/* dd <pd#> [<sector>] - Dump Sector */
					if (argc >= 3)
					{
						drv = atoi(argv[2]);
						if (argc >= 4)
						{
							sec = strtol(argv[3], (char **)NULL, 0);
						}
						gshell_printf_f("rc=%d\r\n",(disk_read(drv, workBuf, sec, 1)));
						gshell_printf_f("Sector: %lu\r\n", sec);
						sec += 1;
						for (dval = 0; dval < 0x200; dval += 16)
						{
							dump_line(&workBuf[dval], dval, 16);
						}
					}
					break;
				case 's':	/* ds <pd#> - Show disk status */
					if (argc >= 3)
					{
						drv = atoi(argv[2]);
						if (disk_ioctl((BYTE)drv, GET_SECTOR_COUNT, &dval) == RES_OK){
							gshell_printf_f("Drive size: %lu sectors\r\n", dval);
						}
						if (disk_ioctl((BYTE)drv, GET_BLOCK_SIZE, &dval) == RES_OK){
							gshell_printf_f("Erase block: %lu sectors\r\n", dval);
						}						
					}
					break;
				case 'c':
					if (argv[1][2] == 's')	/* dcs <pd#> - CTRL_SYNC */
					{
						if (argc >= 3)
						{
							drv = atoi(argv[2]);
							gshell_printf_f("rc=%d\r\n",(disk_ioctl(drv, CTRL_SYNC, 0)));
						}
					}
					break;
			}
			break;
		case 'b':
			switch (argv[1][1])
			{
				case 'd':	/* bd <addr> - Dump R/W Buffer */
					if (argc >= 3)
					{
						drv = strtol(argv[2], (char **)NULL, 0);
						
						for (dval = drv; dval < 0x200; dval += 16)
						{
							dump_line(&workBuf[dval], dval, 16);
						}
					}
					break;
				case 'e':	/* be <addr> ... - Edit R/W buffer */
					if (argc >= 4)
					{
						drv = strtol(argv[2], (char **)NULL, 0);
						
						if ((drv + argc - 3) < 0x200)
						{
							for (dval = 3; dval < argc; dval++)
							{
								dtemp = atoi(argv[dval]);
								workBuf[drv++] = (uint8_t)dtemp;
							}
						}
					}
					break;
				case 'r':	/* br <pd#> <sector> [<n>] - Read Disk into R/W buffer */
					if (argc >= 4)
					{
						drv = atoi(argv[2]);
						dval = strtol(argv[3], (char **)NULL, 0);
						gshell_printf_f("rc=%d\r\n",(disk_read(drv, workBuf, dval, 1)));
					}
					break;
				case 'w':	/* bw <pd#> <sector> [<n>] - Write R/W buffer into disk */
					if (argc >= 4)
					{
						drv = atoi(argv[2]);
						dval = strtol(argv[3], (char **)NULL, 0);
						gshell_printf_f("rc=%d\r\n",(disk_write(drv, workBuf, dval, 1)));
					}
					break;
				case 'f':	/* bf <n> - Fill working buffer */
					if (argc >= 3)
					{
						drv = atoi(argv[2]);
						memset(workBuf, (uint8_t)drv, sizeof(workBuf));
					}
					break;
			}
			break;
		case 'f':
			switch (argv[1][1])
			{
				case 'i':	/* fi <ld#> [<mount>] - Initialize logical drive */
					if (argc >= 3)
					{
						drv = atoi(argv[2]);
						if (argc >= 4)
						{
							dval = atoi(argv[3]);
						}
						else
						{
							dval = 0;
						}
						sprintf(line, "%d:", (int)drv);
						ret_fatfs_result(f_mount(fptr_cli,line, dval));
					}
					break;
				case 's':	/* fs [<path>] - Show logical drive status */
					if (argc >= 3)
					{
						fr = f_getfree(argv[2], &dtemp, &fptr_cli);
						if (fr)
						{
							ret_fatfs_result(fr);
							break;
						}
						gshell_putString_f("FAT type = ");
						gshell_putString_flash(fatfs_type[fptr_cli->fs_type]);
						gshell_printf_f("\r\nBytes/Cluster = %lu\r\n"
							"Number of FATs = %u\r\n"
							"Root DIR entries = %u\r\n"
							,(DWORD)fptr_cli->csize * 512, fptr_cli->n_fats,
							fptr_cli->n_rootdir);
						gshell_printf_f(
							"Sectors/FAT = %lu\r\n"
							"Number of clusters = %lu\r\n"
							"Volume start (lba) = %lu\r\n"
							, fptr_cli->fsize, fptr_cli->n_fatent-2,
							(DWORD)fptr_cli->volbase);
						gshell_printf_f(
							"FAT start (lba) = %lu\r\n"
							"DIR start (lba, cluster) = %lu\r\n"
							"Data start (lba) = %lu\r\n"
							,(DWORD)fptr_cli->fatbase, (DWORD)fptr_cli->dirbase,
							(DWORD)fptr_cli->database);
						
						fr = f_getlabel(argv[2], line, &dval);
						if (fr)
						{
							ret_fatfs_result(fr);
							break;
						}
						if (line[0])
						{
							gshell_printf_f("Volume name is %s\r\n", line);
						}
						else
						{
							gshell_putString_f("No volume label\r\n");
						}
						gshell_printf_f("Volume S/N is %04X-%04X\r\n",(WORD)((DWORD)dval >> 16), (WORD)(dval & 0xFFFF));
						
						gshell_putString_f("...");
						acc_files = acc_dirs = dval = 0;
						strcpy((char*)workBuf, argv[2]);						
						fr = scan_files((char*)workBuf, &acc_dirs, &acc_files, &dval);
						if (fr)
						{
							ret_fatfs_result(fr);
							break;
						}
						gshell_printf_f("\r%u files, %lu bytes.\r\n%u folders\r\n",
									acc_files, dval, acc_dirs);
						gshell_printf_f("%lu KiB total disk space.\r\n%lu KiB available.\r\n",
									(fptr_cli->n_fatent - 2) * (fptr_cli->csize / 2),
									 dtemp * (fptr_cli->csize / 2));
					}
					break;
				case 'l':	/* fl [<path>] - Directory listing */
					if (argc >= 3)
					{
						fr = f_opendir(&Dir, argv[2]);
						if (fr)
						{
							ret_fatfs_result(fr);
							break;
						}
						dval = acc_dirs = acc_files = 0;
						for(;;){
							fr = f_readdir(&Dir, &Finfo);
							if ((fr != FR_OK) || !Finfo.fname[0])	break;
							if (Finfo.fattrib & AM_DIR)
							{
								acc_files++;
							}
							else
							{
								acc_dirs++;
								dval += Finfo.fsize;
							}
							gshell_printf_f(
								"%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\r\n",
								(Finfo.fattrib & AM_DIR) ? 'D' : '-',
								(Finfo.fattrib & AM_RDO) ? 'R' : '-',
								(Finfo.fattrib & AM_HID) ? 'H' : '-',
								(Finfo.fattrib & AM_SYS) ? 'S' : '-',
								(Finfo.fattrib & AM_ARC) ? 'A' : '-',
								(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15,
								Finfo.fdate & 31, (Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
								(DWORD)Finfo.fsize, Finfo.fname);
						}
						if (fr == FR_OK)
						{
							gshell_printf_f("%4u File(s), %6luKiB total\r\n%4u Dir(s)",
								acc_dirs, dval / 1024, acc_files);
							if (f_getfree(argv[2], &dtemp, &fptr_cli) == FR_OK)
							{
								gshell_printf_f(", %6uKiB free\r\n", dtemp * fptr_cli->csize / 2);
							}
						}
						if (fr)		ret_fatfs_result(fr);
					}
					break;
				case 'o':	/* fo <mode> <name> - Open a file */
					if (argc >= 4)
					{
						drv = (BYTE)strtoul(argv[2], NULL, 0);
						ret_fatfs_result(f_open(&File[0], argv[3], (BYTE)drv));
					}
					break;
				case 'c':	/* fc - Close a file */
					ret_fatfs_result(f_close(&File[0]));
					break;
				case 'e':	/* fe <ofs> - Seek file pointer */
					if (argc >= 3)
					{
						dtemp = strtoul(argv[2], NULL, 0);
						fr = f_lseek(&File[0],dtemp);
						ret_fatfs_result(fr);
						if (fr == FR_OK)
						{
							gshell_printf_f(" - fptr = %lu(0x%X)\r\n", (DWORD)File[0].fptr, (DWORD)File[0].fptr);
						}
					}
					else
					{
						gshell_printf_f("fptr = %lu(0x%X)\r\n", (DWORD)File[0].fptr, (DWORD)File[0].fptr);
					}
					break;
				case 'd':	/* fd <len> - read and dump file from current fp */
					if (argc >= 3)
					{
						drv = atoi(argv[2]);
						dtemp = File[0].fptr;
						while(drv){
							if (drv >= 16){
								dcheck = 16;
								drv -= 16;
							} else {
								dcheck = drv;
								drv = 0;
							}
							fr = f_read(&File[0], workBuf, dcheck, &dcheck);
							if (fr != FR_OK)
							{
								ret_fatfs_result(fr);
								break;
							}
							if (!dcheck)	break;
							dump_line(workBuf, dtemp, dcheck);
							dtemp += 16;
						}
					}
					break;
				case 'w':	/* fw - Write Values at filepointer */
					if (argc >= 3)
					{
						for (uint8_t i = 2; i < argc; i++)
						{
							line[i - 2] = strtoul(argv[i], NULL, 0);
						}
						fr = f_write(&File[0], line, argc - 3, &drv);
						ret_fatfs_result(fr);
						if (fr != FR_OK)
						{
							break;
						}
						if (drv > (argc - 2))
						{
							gshell_putString_f(" - Memory full?\r\n");
							break;
						}
						gshell_printf_f(" - %lu Bytes written, fptr now at %lu(0x%X)\r\n", (uint8_t)drv, (DWORD)File[0].fptr);
					}
					break;
				case 'p':	/* fp - Write Value at position */
					if (argc >= 4)
					{
						drv = strtoul(argv[2],NULL, 0);
						u8temp = (uint8_t)strtoul(argv[3], NULL, 0);
						
						fr = f_lseek(&File[0], drv);
						if (fr != FR_OK)
						{
							ret_fatfs_result(fr);
							break;
						}
						fr = f_write(&File[0], &u8temp, 1, &drv);
						ret_fatfs_result(fr);
						if (fr != FR_OK)
						{
							break;
						}
						if (u8temp > drv)
						{
							gshell_putString_f(" - Memory full?\r\n");
							break;
						}
						gshell_printf_f(" - Byte 0x%02X written, fptr now at %lu(0x%1X)\r\n", (uint8_t)u8temp, (DWORD)File[0].fptr)
					}
					break;
				case 'v':	/* fv - Truncate file */
					ret_fatfs_result(f_truncate(&File[0]));
					break;
				case 'n':	/* fn <old_name> <new_name> - Change file/dir name */
					if (argc >= 4)
					{
						ret_fatfs_result(f_rename(argv[2], argv[3]));
					}
					break;
				case 'u':	/* fu <name> - Unlink a file or dir */
					if (argc >= 3)
					{
						ret_fatfs_result(f_unlink(argv[2]));
					}
					break;
				case 'k':	/* fk <name> - Create a directory */
					if (argc >= 3)
					{
						ret_fatfs_result(f_mkdir(argv[2]));
					}
					break;
				case 'x':	/* fx <src_name> <dst_name> - Copy file */
					if (argc >= 4)
					{
						gshell_printf_f("Opening \"%s\"\r\n", argv[2]);
						fr = f_open(&File[0], argv[2], FA_OPEN_EXISTING | FA_READ);
						if (fr)
						{
							ret_fatfs_result(fr);
							break;
						}
						gshell_printf_f("Creating \"%s\"\r\n", argv[3]);
						fr = f_open(&File[1], argv[3], FA_CREATE_ALWAYS | FA_WRITE);
						if (fr)
						{
							ret_fatfs_result(fr);
							break;
						}
						gshell_putString_f("Copying...\r\n");
						dval = 0;
						dtemp = gf_millis();
						for (;;)
						{
							fr = f_read(&File[0], workBuf, FF_MAX_SS, &drv);
							if (fr || drv == 0)	break;
							fr = f_write(&File[1], workBuf, drv, &acc_files);
							dval += acc_files;
							if (fr || acc_files < drv)	break;
						}
						dtemp -= gf_millis();
						gshell_printf_f("%lu Bytes copied at %lu Bytes/Sec.\r\n", 
							dval, dval * 1000 / (dtemp) ? (dtemp) : (1));
						f_close(&File[0]);
						f_close(&File[1]);
					}
					break;
				case 'b':	/* fb <name> - Set volume label */
					if (argc >= 3)
					{
						ret_fatfs_result(f_setlabel(argv[2]));
					}
					break;
				case 'm':	/* fm <ld#> [<fs type [<au_size> [<align> [<n_fats> [<n_root>]]]]] - Create filesystem */
					if (format_unlocked)
					{
						if (argc >= 3)
						{
							drv = atoi(argv[2]);
							MKFS_PARM opt, *popt = 0;
							if (argc >= 4)
							{
								dtemp = atoi(argv[3]);
								memset(&opt, 0, sizeof(opt));
								popt = &opt;
								popt->fmt = (BYTE)dtemp;
								if (argc >= 5)
								{
									dtemp = atoi(argv[4]);
									popt->au_size = dtemp;
									if (argc >= 6)
									{
										dtemp = atoi(argv[5]);
										popt->align = dtemp;
										if (argc >= 7)
										{
											dtemp = atoi(argv[6]);
											popt->n_fat = (BYTE)dtemp;
											if (argc >= 8)
											{
												dtemp = atoi(argv[7]);
												popt->n_root = dtemp;
											}
										}
									}
								}
							}
							gshell_printf_f("The drive %u will be formatted...", (WORD)drv);
							sprintf(line, "%u:", (BYTE)drv);
							ret_fatfs_result(f_mkfs(line, popt, workBuf, FF_MAX_SS));
						}
						format_unlocked = 0;
					}
					else
					{
						gshell_putString_f("Formatting locked! use 'format unlock' to enable this command\r\n");
					}
					break;					
			}
			break;
		case '?':	/* Show command list */
			gshell_putString_f(
				G_TEXTBOLD"[Disk controls]"G_TEXTNORMAL"\r\n"
				" dd [<pd#> <sect>]        - Dump a sector\r\n"
				" ds <pd#>                 - Show disk status\r\n"
				" dcs <pd#>                - ioctl(CTRL_SYNC)\r\n"
				G_TEXTBOLD"[Buffer Controls]"G_TEXTNORMAL"\r\n"
				" bd <ofs>                 - Dump working buffer\r\n"
				" be <ofs> [<data>] ...    - Edit working buffer\r\n"
				" br <pd#> <sect> [<num>]  - Read disk into working buffer\r\n"
				" bw <pd#> <sect> [<num>]  - Write working buffer into disk\r\n"
				" bf <val>                 - Fill working buffer\r\n"
				G_TEXTBOLD"[Filesysem controls]"G_TEXTNORMAL"\r\n"
				" fi <ld#> [<mount>]       - Force initialize the volume/drive\r\n"
				" fs [<path>]              - Show volume status\r\n"
				" fl [<path>]              - Show a directory\r\n"
				" fo <mode> <file>         - Open a file\r\n"
				" fc                       - Close the file\r\n"
				" fe [<ofs>]               - Move fp in normal seek or returns it\r\n"
				" fd <len>                 - Read and dump the file\r\n"
				" fw <value> [value...]    - Writes a Byte the set file pointer\r\n"
				" fp <addr> <value>        - Writes a Byte at the said position into the file\r\n"
				" fv                       - Truncate the file at current fp\r\n"
				" fn <og_name> <new_name>  - Rename an object\r\n"
				" fu <obj_name>            - Unlink an object\r\n"
				" fk <dir name>            - Create a directory\r\n"
				" fx <src file> <dst file> - Copy a file\r\n"
				" fb <name>                - Set volume label\r\n"
				" format unlock            - Unlocks the ability to format / create filesystem\r\n"
				" fm <ld#> [<fs type> [<au_size> [<align> [<n_fats> [<n_root>]]]]]\r\n"
				"                                        - Format volume and create filesystem\r\n"
				);
			break;
		default:
			gshell_putString_f("Unknown command!\r\nType 'fatfs ?' to show the commands\r\n");
			break;			
	}
}

static void cli_cmd_rtc(uint8_t argc, char *argv[])
{
	time_t curTime;
	struct tm rtc_time;
	RV3028_TIME_t rtcTime;
	char *token;
	if ((argc > 2) && (strcmp_PF(argv[1], (__uint24)XSTR("set")) == 0))
	{
		token = strtok(argv[1], ":");
		rtc_time.tm_hour = atoi(token);
		token = strtok(NULL, ":");
		rtc_time.tm_min = atoi(token);
		token = strtok(NULL, ":");
		rtc_time.tm_sec = atoi(token);
		
		token = strtok(argv[2], ".");
		rtc_time.tm_mday = atoi(token);
		token = strtok(NULL, ".");
		rtc_time.tm_mon = atoi(token);
		token = strtok(NULL, ".");
		rtc_time.tm_year = (atoi(token) - 1900);
		
		set_system_time(mktime((struct tm*)&rtc_time));
		rtcTime.seconds = rtc_time.tm_sec;
		rtcTime.minutes = rtc_time.tm_min;
		rtcTime.hours = rtc_time.tm_hour;
		rtcTime.weekday = rtc_time.tm_wday;
		rtcTime.month = rtc_time.tm_mon + 1;
		rtcTime.date = rtc_time.tm_mday;
		rtcTime.year = rtc_time.tm_year - 100;
		
		rv3028_setTime(&(mcuBoard->rtcClk), &rtcTime);
	}
	else if ((argc > 1) && (strcmp_PF(argv[1], (__uint24)XSTR("get")) == 0))
	{
		time(&curTime);
		gshell_putString(ctime(&curTime));
		gshell_putString_f("\r\n");
	}
	else
	{
		gshell_putString_f("RTC/Time Module\r\n"
			"Arguments:\r\n"
			" get                       - Get the current time of the RTC module\r\n"
			" set <HH:MM:SS DD.MM.YYYY> - Set the time of the RTC module\r\n");
	}
}

static void cli_cmd_rx(uint8_t argc, char *argv[])
{
	FRESULT fr;
	FIL fdst;
	uint32_t dtemp;
	uint8_t xr;
	FATFS *fs;
	uint32_t msWait;
	uint32_t timeStamp;
	
	if (argc > 1)
	{
		fr = f_open(&fdst, argv[1], FA_CREATE_NEW | FA_WRITE);
		if (fr)
		{
			ret_fatfs_result(fr);
			return;
		}
		fr = f_getfree("0:", &dtemp, &fs);
		if (fr)
		{
			ret_fatfs_result(fr);
			return;
		}
		
		dtemp = dtemp * fs->csize / 2 * 1024;
		gshell_putString_f("Start the X-Modem Sender now...\r\n");
		msWait = gf_millis();
		while((msWait + 5000) >= gf_millis()){}
		timeStamp = gf_millis();
		xr = xmodem_receive(&fdst, &dtemp);
		timeStamp = gf_millis() - timeStamp;
		f_close(&fdst);
		if (xr)
		{
			gshell_printf_f("\r\nReceiving failed with error code %d\r\n", xr);
			f_unlink(argv[1]);
			return;
		}
		msWait = gf_millis();
		while((msWait + 1000) >= gf_millis()){}
		gshell_printf_f("File received!\r\n%lu Bytes written in %lu seconds - %lu B/s\r\n",
			dtemp, timeStamp / 1000, dtemp / (timeStamp / 1000));
		gshell_printf_f("File saved at %s\r\n", argv[1]);
	}
	else
	{
		gshell_putString_f("X-Modem receive module\r\n"
		"Usage:\r\n"
		" rx <filename.foo>   - Activates the X-Modem receiver and saves it on the flash chip\r\n");
	}
}

static void cli_cmd_bmp(uint8_t argc, char *argv[])
{
	FRESULT fr;
	FIL fdst;
	uint32_t x0, y0;
	uint32_t msWait;
	uint32_t dispAddress;
	if (argc > 3)
	{
		x0 = atoi(argv[1]);
		y0 = atoi(argv[2]);
		
		if ((x0 > RA8876_WIDTH) || (y0 > RA8876_HEIGHT))
		{
			gshell_putString_f("Coordinates too large!\r\n");
			return;
		}
		
		msWait = gf_millis();
		
		fr = f_open(&fdst, argv[3], FA_READ);
		if (fr)
		{
			f_close(&fdst);
			ret_fatfs_result(fr);
			return;
		}
		
		dispAddress = RA8876_readReg(RA8876_MISA0);
		dispAddress |= (uint32_t)RA8876_readReg(RA8876_MISA1) << 8;
		dispAddress |= (uint32_t)RA8876_readReg(RA8876_MISA2) << 16;
		dispAddress |= (uint32_t)RA8876_readReg(RA8876_MISA3) << 24;
		
		
		RAHelper_loadBitmapFromFIL(dispAddress,(uint16_t)x0, (uint16_t)y0, &fdst);
		msWait = gf_millis() - msWait;
		gshell_printf_f("Image loaded in %"PRIu32" milliseconds!\r\n", msWait);
		f_close(&fdst);
	}
	else
	{
		gshell_putString_f("Bitmap Display Tester\r\n"
		"Usage:\r\n"
		" bmp <x> <y> <image.bmp>   - Display a Bitmap Image at position x/y\r\n");
	}
}

static void cli_cmd_restart(uint8_t argc, char* argv[])
{
	gshell_putString_f("Issuing software reset..."G_CRLF G_CRLF);
	_PROTECTED_WRITE(RST.CTRL, RST_WDRF_bm);
}

static const gshell_cmd_t shell_command_list[] = {
	{G_XARR("fatfs"),	cli_cmd_fatfs,	G_XARR("FatFs Module Test Monitor")},
	{G_XARR("rtc"),		cli_cmd_rtc,	G_XARR("RTC / Time Command")},
	{G_XARR("rx"),		cli_cmd_rx,		G_XARR("Receive using the X-Modem Protocol")},
	{G_XARR("bmp"),		cli_cmd_bmp,	G_XARR("Load BMP from memory to display")},
	{G_XARR("reset"),	cli_cmd_restart,G_XARR("Restarts the microcontroller")}
};

const gshell_cmd_t *const gshell_list_commands = shell_command_list;
const uint8_t gshell_list_num_commands = sizeof(shell_command_list) / sizeof(shell_command_list[0]);