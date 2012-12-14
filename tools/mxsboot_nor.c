#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

typedef unsigned int __u32;
typedef __u32 uint32_t;
/*
 * The following data structures are copied from:
 * i.MX28 Applications Processor Reference Manual, Rev. 1, 2010
 * page 973 - 974
 */

/* brief Spi media configuration block structs */
typedef struct _spi_ConfigBlockFlags
{
	/*
	 * Ignored for Spis
	 * 0 - Do not disable fast reads
	 * 1 - Disable fast reads
	 */
	uint32_t DisableFastRead:1;
} spi_ConfigBlockFlags_t;

typedef struct _ssp_clockConfig
{
	/* Clock Select (0=io_ref 1=xtal_ref) */
	int clkSel:1;
	/* IO FRAC 18-35 (io_frac+16) */
	int io_frac:6;
	/* SSP FRAC (1=default) */
	int ssp_frac:9;
	/* Divider: Must be an even value 2-254 */
	int ssp_div:8;
	/* Serial Clock Rate */
	int ssp_rate:8;
} ssp_ClockConfig_t;

typedef struct _spi_ConfigBlockClocks
{
	/* sizeof(ssp_ClockConfig_t) */
	uint32_t SizeOfSspClockConfig;
	/* 
	 * SSP clock configuration structure. A null
	 * structure indicates no clock change.
	 */
	ssp_ClockConfig_t SspClockConfig;
} spi_ConfigBlockClocks_t;


/* Little Endian */
typedef struct _spi_ConfigBlock
{
	uint32_t Signature;
	uint32_t BootStartAddr;
	uint32_t SectorSize;
	spi_ConfigBlockFlags_t Flags;
	spi_ConfigBlockClocks_t Clocks;
	uint32_t pad[6];
} spi_ConfigBlock_t;

/*
 * EN25Q80A: sector size is 256 bytes
 */
#define SECTOR_SIZE 256

/*
 * i.MX28 Applications Processor Reference Manual, Rev. 1, 2010
 * page 974
 */
#define SCP_SIGNATURE 0x4D454D53

static void init_config_block(spi_ConfigBlock_t *scp, uint32_t SectorSize)
{
	if(!scp)
		return;

	memset(scp, 0, sizeof(spi_ConfigBlock_t));
	scp->Signature = SCP_SIGNATURE;
	scp->BootStartAddr = sizeof(spi_ConfigBlock_t);
	scp->SectorSize = SectorSize;

	scp->Clocks.SizeOfSspClockConfig =  sizeof(ssp_ClockConfig_t);
}


static void print_usage(void)
{
	printf("Usage: \n"
			"mxsboot_nor [-s <sector size>] -i <infile> -o <outfile>\n");
	printf(
		"BootStream file with a proper header for i.MX28 SPI FLASH boot\n"
		"\n"
		"  <sector size> the sector size of your NOR FLASH, the default value is 256\n"
		"  <infile>      input file, the u-boot.sb bootstream\n"
		"  <outfile>     output file, the bootable image\n"
		"\n");
	printf("-------------------------------------------\n");
}


int main(int argc, char *argv[])
{
	spi_ConfigBlock_t sc;
	int infd;
	int outfd;
	spi_ConfigBlock_t *scp;
	int size = 0;
	int total;
	char *inname;
	char *outname;
	static char buf[1024];
	int c;
	int sector_size = SECTOR_SIZE;
	int ret = 0;

	if(argc < 5){
		print_usage();
		ret = -1;
		goto out;
	}

	while ((c = getopt(argc, argv, "i:o:s:")) != EOF) {
		switch (c) {
		case 'i':
			inname = optarg;
			break;
		case 'o':
			outname = optarg;
			break;
		case 's':
			if(!optarg)
				break;

			sector_size = atoi(optarg);
			if ((sector_size <= 0) || (sector_size % 2)){
				printf("Invalid sector size.\n");
				print_usage();
				ret = -1;
				goto out;
			}
			break;
		default:
			print_usage();
			ret = -1;
			goto out;
		}
	}

	if(!inname || !outname){
		print_usage();
		ret = -1;
		goto out;
	}

	infd = open(inname, O_RDONLY);
	if(infd < 0){
		printf("Cannot open the sb image: %s. Error: %s\n", inname, strerror(errno));
		ret = -1;
		goto out;
	}

	outfd = open(outname, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if(outfd < 0){
		printf("cannot create the NOR flash image: %s. Error: %s\n", outname, strerror(errno));
		ret = -1;
		goto inerrout;
	}

	scp = &sc;
	init_config_block(scp, sector_size);
	
	/* write config block */
	if(write(outfd, (char *)scp, sizeof(spi_ConfigBlock_t)) < 0){
		printf("Cannot write config block to NOR flash image:%s. Error: %s\n", outname, strerror(errno));
		ret = -1;
		goto outerrout;
	}

	total = lseek(infd, 0, SEEK_END);
	lseek(infd, 0, SEEK_SET);

	while(size < total){
		int len;
		memset(buf, 0xff, sizeof(buf));
		len = read(infd,  buf, sizeof(buf));
		if(len < 0){
			printf("Read sb image: %s failed! Error: %s.\n", inname, strerror(errno));
			ret = -1;
			break;
		}
		if(write(outfd, buf, len) < 0){
			printf("Write NOR flash image: %s failed! Error: %s\n", outname, strerror(errno));
			ret = -1;
			break;
		}
		size += len;
	}
	
outerrout:	
	close(outfd);
inerrout:	
	close(infd);
out:	
	return ret;
}
