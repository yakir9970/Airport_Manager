#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "Company.h"
#include "Airport.h"
#include "General.h"
#include "fileHelper.h"
#include "myMacros.h"

static const char* sortOptStr[eNofSortOpt] = {
	"None","Hour", "Date", "Airport takeoff code", "Airport landing code" };


int	initCompanyFromFile(Company* pComp, AirportManager* pManaer, const char* fileName)
{
#ifdef COMPRESSION
	FILE* bFile = fopen(fileName,"rb");
	CHECK_MSG_RETURN_0(bFile, error loading\n);
	
	unsigned char buffer[10];
	fread(buffer, sizeof(char), 2, bFile);
	pComp->flightCount = (buffer[1]) << 1 | (buffer[0] >> 7);
	pComp->sortOpt = buffer[0] >> 4 & 0x7;
	int lenght = buffer[0] & 0xF;
	pComp->name = (char*)malloc((lenght+1) * sizeof(char));  
	CHECK_RETURN_0(pComp->name);
	
	fread(pComp->name, sizeof(char), lenght+1, bFile); 

	char checker[CODE_LENGTH+1];
	
	pComp->flightArr = (Flight**)malloc(pComp->flightCount * sizeof(Flight*));
	for (int i = 0; i < pComp->flightCount; i++)
	{
		fread(buffer, sizeof(char), 10, bFile);
		pComp->flightArr[i] = (Flight*)calloc(1, sizeof(Flight));
		CHECK_NULL__MSG_COLSE_FILE(pComp->flightArr[i], Alocation error\n, bFile);
		
		checker[0] = buffer[0];
		checker[1] = buffer[1];
		checker[2] = buffer[2];
		checker[3] = '\0';
		Airport* checkerA = findAirportByCode(pManaer, checker);
		CHECK_RETURN_0(checkerA);
		
		strcpy(pComp->flightArr[i]->originCode, checker);

		checker[0] = buffer[3];
		checker[1] = buffer[4];
		checker[2] = buffer[5];
		checker[3] = '\0';
		checkerA = findAirportByCode(pManaer, checker);
		CHECK_RETURN_0(checkerA);
		strcpy(pComp->flightArr[i]->destCode, checker);

		pComp->flightArr[i]->date.year = (buffer[9]<<10) | (buffer[8] << 2)| (buffer[7] >> 6);
		pComp->flightArr[i]->date.month = buffer[7] >> 2 & 0xF;
		pComp->flightArr[i]->date.day = ((buffer[7] & 0x3) << 3) | (buffer[6] >> 5);
		pComp->flightArr[i]->hour = buffer[6] & 0x1F;
	}
	
	L_init(&pComp->flighDateList);
	initDateList(pComp);
	return 1;
#else
	L_init(&pComp->flighDateList);
	if (loadCompanyFromFile(pComp, pManaer, fileName))
	{
		initDateList(pComp);
		return 1;
	}
#endif // COMPRESSION
	return 0;
}

void	initCompany(Company* pComp, AirportManager* pManaer)
{
	printf("-----------  Init Airline Company\n");
	L_init(&pComp->flighDateList);

	pComp->name = getStrExactName("Enter company name");
	pComp->flightArr = NULL;
	pComp->flightCount = 0;
}

void	initDateList(Company* pComp)
{
	for (int i = 0; i < pComp->flightCount; i++)
	{
		if (isUniqueDate(pComp, i))
		{
			char* sDate = createDateString(&pComp->flightArr[i]->date);
			L_insert(&(pComp->flighDateList.head), sDate);
		}
	}
}

int		isUniqueDate(const Company* pComp, int index)
{
	Date* pCheck = &pComp->flightArr[index]->date;
	for (int i = 0; i < pComp->flightCount; i++)
	{
		if (i == index)
			continue;
		if (equalDate(&pComp->flightArr[i]->date, pCheck))
			return 0;
	}
	return 1;
}

int		addFlight(Company* pComp, const AirportManager* pManager)
{

	if (pManager->count < 2)
	{
		printf("There are not enoght airport to set a flight\n");
		return 0;
	}
	pComp->flightArr = (Flight**)realloc(pComp->flightArr, (pComp->flightCount + 1) * sizeof(Flight*));
	if (!pComp->flightArr)
		return 0;
	pComp->flightArr[pComp->flightCount] = (Flight*)calloc(1, sizeof(Flight));
	if (!pComp->flightArr[pComp->flightCount])
		return 0;
	initFlight(pComp->flightArr[pComp->flightCount], pManager);
	if (isUniqueDate(pComp, pComp->flightCount))
	{
		char* sDate = createDateString(&pComp->flightArr[pComp->flightCount]->date);
		L_insert(&(pComp->flighDateList.head), sDate);
	}
	pComp->flightCount++;
	return 1;
}

void	printCompany(const Company* pComp,  ...) 
{
	if (pComp == NULL)
	{
		printf("Error reading company");
		return;
	}

	va_list   strings;
	va_start(strings, pComp);
	printf("Company %s", pComp->name);
	char* temp = va_arg(strings, char*);

	while (temp != NULL)
	{
		printf("_%s", temp);
		temp = va_arg(strings, char*);
	}
	va_end(strings);
	printf(" ");

#ifdef DETAIL_PRINT
	printf("Has %d flights\n", pComp->flightCount);
	generalArrayFunction((void*)pComp->flightArr, pComp->flightCount, sizeof(Flight**), printFlightV);
	printf("\nFlight Date List:");
	L_print(&pComp->flighDateList, printStr);
#else
	printf("Has %d flights\n", pComp->flightCount);
#endif
}

void	printFlightsCount(const Company* pComp)
{
	char codeOrigin[CODE_LENGTH + 1];
	char codeDestination[CODE_LENGTH + 1];

	if (pComp->flightCount == 0)
	{
		printf("No flight to search\n");
		return;
	}

	printf("Origin Airport\n");
	getAirportCode(codeOrigin);
	printf("Destination Airport\n");
	getAirportCode(codeDestination);

	int count = countFlightsInRoute(pComp->flightArr, pComp->flightCount, codeOrigin, codeDestination);
	if (count != 0)
		printf("There are %d flights ", count);
	else
		printf("There are No flights ");

	printf("from %s to %s\n", codeOrigin, codeDestination);
}



int		saveCompanyToFile(const Company* pComp, const char* fileName)
{
#ifdef COMPRESSION
	int nameSize = strlen(pComp->name);
	int size = 2 + nameSize;
	
	unsigned char* data = (unsigned char*)calloc(sizeof(unsigned char), size+1);
	data[1] = (pComp->flightCount >> 1);
	data[0] = (pComp->flightCount << 7) | (pComp->sortOpt << 4) | (nameSize);
	for (int i = 2; i < size; i++)
	{
		data[i] = pComp->name[i-2];
	}
	data[size] = '\0';

	FILE* bFile=fopen(fileName,"wb");
	CHECK_RETURN_0(bFile);
	
	fwrite(data, sizeof(char), size+1, bFile);

	data = (char*)realloc(data, sizeof(char)*10);
	for (int i = 0; i < pComp->flightCount; i++)
	{
		for (int j = 0; j < CODE_LENGTH; j++)
		{
			data[j] = pComp->flightArr[i]->originCode[j];
			data[j+3] = pComp->flightArr[i]->destCode[j];
		}
		data[9] = pComp->flightArr[i]->date.year >> 10;
		data[8] = (pComp->flightArr[i]->date.year >> 2) & 0xFF;
		data[7] = (pComp->flightArr[i]->date.year & 0x3) << 6 | (pComp->flightArr[i]->date.month << 2) | (pComp->flightArr[i]->date.day >> 3);
		data[6] = (pComp->flightArr[i]->date.day << 5) | (pComp->flightArr[i]->hour);

		fwrite(data, sizeof(char), 10, bFile);
	}
	
	free(data);
	fclose(bFile);
	return 1;
	
#else
	FILE* fp;
	fp = fopen(fileName, "wb");
	CHECK_NULL__MSG_COLSE_FILE(fp, Error open copmpany file to write\n);


	if (!writeStringToFile(pComp->name, fp, "Error write comapny name\n"))
		return 0;

	if (!writeIntToFile(pComp->flightCount, fp, "Error write flight count\n"))
		return 0;

	if (!writeIntToFile((int)pComp->sortOpt, fp, "Error write sort option\n"))
		return 0;

	for (int i = 0; i < pComp->flightCount; i++)
	{
		if (!saveFlightToFile(pComp->flightArr[i], fp))
			return 0;
	}

	fclose(fp);
	return 1;
#endif // COMPRESSION


}

int loadCompanyFromFile(Company* pComp, const AirportManager* pManager, const char* fileName)
{
	FILE* fp;
	fp = fopen(fileName, "rb");
	CHECK_MSG_RETURN_0(fp, Error open company file\n);
	
	pComp->flightArr = NULL;
	

	pComp->name = readStringFromFile(fp, "Error reading company name\n");
	CHECK_RETURN_0(pComp->name);
	

	if (!readIntFromFile(&pComp->flightCount, fp, "Error reading flight count name\n"))
		return 0;

	int opt;
	if (!readIntFromFile(&opt, fp, "Error reading sort option\n"))
		return 0;

	pComp->sortOpt = (eSortOption)opt;

	if (pComp->flightCount > 0)
	{
		pComp->flightArr = (Flight**)malloc(pComp->flightCount * sizeof(Flight*));
		if (!pComp->flightArr)
		{
			MSG_CLOSE_RETURN_0(fp, Alocation error\n);
		}
	}
	else
		pComp->flightArr = NULL;

	for (int i = 0; i < pComp->flightCount; i++)
	{
		pComp->flightArr[i] = (Flight*)calloc(1, sizeof(Flight));
		if (!pComp->flightArr[i])
		{
			MSG_CLOSE_RETURN_0(fp, Alocation error\n);
		}
		if (!loadFlightFromFile(pComp->flightArr[i], pManager, fp))
			return 0;
	}

	fclose(fp);
	return 1;
}

void	sortFlight(Company* pComp)
{
	pComp->sortOpt = showSortMenu();
	int(*compare)(const void* air1, const void* air2) = NULL;

	switch (pComp->sortOpt)
	{
	case eHour:
		compare = compareByHour;
		break;
	case eDate:
		compare = compareByDate;
		break;
	case eSorceCode:
		compare = compareByCodeOrig;
		break;
	case eDestCode:
		compare = compareByCodeDest;
		break;

	}

	if (compare != NULL)
		qsort(pComp->flightArr, pComp->flightCount, sizeof(Flight*), compare);

}

void	findFlight(const Company* pComp)
{
	int(*compare)(const void* air1, const void* air2) = NULL;
	Flight f = { 0 };
	Flight* pFlight = &f;


	switch (pComp->sortOpt)
	{
	case eHour:
		f.hour = getFlightHour();
		compare = compareByHour;
		break;
	case eDate:
		getchar();
		getCorrectDate(&f.date);
		compare = compareByDate;
		break;
	case eSorceCode:
		getchar();
		getAirportCode(f.originCode);
		compare = compareByCodeOrig;
		break;
	case eDestCode:
		getchar();
		getAirportCode(f.destCode);
		compare = compareByCodeDest;
		break;
	}

	if (compare != NULL)
	{
		Flight** pF = bsearch(&pFlight, pComp->flightArr, pComp->flightCount, sizeof(Flight*), compare);
		if (pF == NULL)
			printf("Flight was not found\n");
		else {
			printf("Flight found, ");
			printFlight(*pF);
		}
	}
	else {
		printf("The search cannot be performed, array not sorted\n");
	}

}

eSortOption showSortMenu()
{
	int opt;
	printf("Base on what field do you want to sort?\n");
	do {
		for (int i = 1; i < eNofSortOpt; i++)
			printf("Enter %d for %s\n", i, sortOptStr[i]);
		scanf("%d", &opt);
	} while (opt < 0 || opt >eNofSortOpt);

	return (eSortOption)opt;
}

void	freeCompany(Company* pComp)
{
	generalArrayFunction((void*)pComp->flightArr, pComp->flightCount, sizeof(Flight**), freeFlight);
	free(pComp->flightArr);
	free(pComp->name);
	L_free(&pComp->flighDateList, freePtr);
}
