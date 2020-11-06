#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <time.h>
#include <vector>

#include "SiftGPU/SiftGPU.h"
#include "GL/glew.h"
#include "gdal_priv.h"

#include "SIFTFileIO.hpp"
using namespace std;

#ifdef WIN32
#include <windows.h>
#include <ppl.h>
#define PARALLEL_SORT Concurrency::parallel_sort
#else
#include <algorithm.h>
#define PARALLEL_SORT std::sort
#endif // WIN32


#pragma warning(disable:4251)
#pragma warning(disable:4996)

inline bool GetCurrentPath(char buf[], int len)
{
#ifdef WIN32
	GetModuleFileName(NULL, buf, len);
	return true;
#else
	int n = readlink("/proc/self/exe", buf, len);
	if (n > 0 && n < sizeof(buf)) {
		return true;
	}
	else return false;
#endif
}

inline char *GetIniKeyString(char *title, char *key, char *filename)
{
	FILE *fp;
	char szLine[1024];
	static char tmpstr[1024];
	int rtnval;
	int i = 0;
	int flag = 0;
	char *tmp;
	if ((fp = fopen(filename, "r")) == NULL) {
		return "";
	}

	while (!feof(fp)) {
		rtnval = fgetc(fp);
		if (rtnval == EOF) {
			break;
		}
		else {
			szLine[i++] = rtnval;
		}

		if (rtnval == '\n') {
#ifndef WIN32
			i--;
#endif  
			szLine[--i] = '\0';
			i = 0;
			tmp = strchr(szLine, '=');
			if ((tmp != NULL) && (flag == 1)) {
				if (strstr(szLine, key) != NULL) {
					//注释行

					if ('#' == szLine[0]) {
					}
					else if ('\/' == szLine[0] && '\/' == szLine[1]) {
					}
					else {
						strcpy(tmpstr, tmp + 1);
						fclose(fp);
						return tmpstr;
					}
				}
			}
			else
			{
				strcpy(tmpstr, "[");
				strcat(tmpstr, title);
				strcat(tmpstr, "]");
				if (strncmp(tmpstr, szLine, strlen(tmpstr)) == 0) {
					//找到title         
					flag = 1;
				}
			}
		}
	}
	fclose(fp);

	return "";
}

inline char *GetFileDirectory(const char* filename) {
	char *ret = NULL;
	char dir[1024];
	char *cur;
	if (filename == NULL) return(NULL);

#if defined(WIN32) && !defined(__CYGWIN__)  
#   define IS_SEP(ch) ((ch=='/')||(ch=='\\'))  
#else  
#   define IS_SEP(ch) (ch=='/')  
#endif  
	strncpy(dir, filename, 1023);
	dir[1023] = 0;
	cur = &dir[strlen(dir)];
	while (cur > dir)
	{
		if (IS_SEP(*cur)) break;

		cur--;
	}
	if (IS_SEP(*cur))
	{
		if (cur == dir)
		{
			//1.根目录  
			dir[1] = 0;
		}
		else
		{
			*cur = 0;
		}
		ret = strdup(dir);
	}
	else
	{
		//1.如果是相对路径,获取当前目录  
		//io.h  
		if (getcwd(dir, 1024) != NULL)
		{
			dir[1023] = 0;
			ret = strdup(dir);
		}
	}
	strcat(ret, "\\");
	return ret;
#undef IS_SEP  
}

int g_sift_point_kept = 2 << 13;
bool g_save_sift_as_bin = true;
bool g_save_sift_ascii_descriptor = false;

void Usage(){
	printf("Function: Extra Feature Point\n"
		"Parameter List:\n"
		"Param1 @ image path\n"
		"Param2 @ feature path\n"
		"Param3 @ block size\n");
	exit(0);
}

void ExtraFeatPt(const char *lpImgPath, const char* lpOutFeaPath, int nBlockSize = 5000);

void loadIni() {
	char ini_path[1024];
	GetCurrentPath(ini_path, 1024);
	char *ps = strrchr(ini_path, '.');
	if (!ps) return;
	strcpy(ps, ".ini");
	char * temp = nullptr;

	temp = GetIniKeyString("SIFTEXTRACT", "SIFT_KEPT_NUM", ini_path);
	if (temp && strlen(temp) > 0) {
		sscanf(temp, "%d", &g_sift_point_kept);
	}

	temp = GetIniKeyString("SIFTEXTRACT", "SAVE_BIN", ini_path);
	if (temp && strlen(temp) > 0) {
		sscanf(temp, "%d", &g_save_sift_as_bin);
	}

	temp = GetIniKeyString("SIFTEXTRACT", "SAVE_ASCII_DESC", ini_path);
	if (temp && strlen(temp) > 0) {
		sscanf(temp, "%d", &g_save_sift_ascii_descriptor);
	}
}

//#define SIFT_BIN_TO_ASCII2
//#define SIFT_RARYFY
int main(int argc, char **argv)
{
#ifdef SIFT_BIN_TO_ASCII
	if (argc != 3 && argc != 4) {
		printf("usage1: input.key output.txt (for left top)\n");
		printf("usage2: input.key output.txt height (for left bottom)\n");
		return 1;
}
	std::vector<LiDARReg::SIFT_KEY_POINT > keypioints;
	LiDARReg::loadSIFTBinFile(argv[1], keypioints);
	FILE* fp = fopen(argv[2], "w");
	if (!fp) return 1;
	int height = 0;
	if (argc > 3) {
		sscanf(argv[3], "%d", &height);
	}
	for (size_t i = 0; i < keypioints.size(); i++) {
		double x = keypioints[i].keys[0];
		double y = keypioints[i].keys[1];
		if (height > 0) {
			y = height - y;
		}
		fprintf(fp, "%d %lf %lf %lf %lf %lf\n", i, 0.0f, 0.0f, -9999.f, x, y);
	}
	fclose(fp);
	return 0;
#endif // SIFT_BIN_TO_ASCII

#ifdef SIFT_BIN_TO_ASCII2
	if (argc != 3) {
		printf("usage1: input.key output.txt (for left top)\n");
		return 1;
	}
	std::vector<LiDARReg::SIFT_KEY_POINT > keypioints;
	LiDARReg::loadSIFTBinFile(argv[1], keypioints);
	FILE* fp = fopen(argv[2], "w");
	if (!fp) return 1;
	fprintf(fp, "%d %d\n", keypioints.size(), 128); //行,列
	for (size_t i = 0; i < keypioints.size(); i++) {
		double x = keypioints[i].keys[0];
		double y = keypioints[i].keys[1];
		double s = keypioints[i].keys[2];
		double o = keypioints[i].keys[3];
		
		fprintf(fp, "%.2f %.2f %.3f %.3f\n", y, x, s, o); //行,列

// 		for (int kd = 0; kd < 128; kd++)
// 		{
// 			fprintf(fp, "%d ", keypioints[i].descriptors[kd]);
// 
// 			if ((kd + 1) % 20 == 0)
// 			{
// 				fprintf(fp, "\n");
// 			}
// 		}
// 		fprintf(fp, "\n");
	}
	fclose(fp);
	return 0;
#endif // SIFT_BIN_TO_ASCII

#ifdef SIFT_RARYFY
	loadIni();
	std::vector<LiDARReg::SIFT_KEY_POINT > keypioints;
	if (g_save_sift_as_bin) {
		LiDARReg::loadSIFTBinFile(argv[1], keypioints);
	}
	else {
		LiDARReg::loadSIFTASCIIFile(argv[1], keypioints);
	}
	if (g_sift_point_kept > 0 && keypioints.size() > g_sift_point_kept)
	{
		keypioints.resize(g_sift_point_kept);
	}
	if (g_save_sift_as_bin) {
		LiDARReg::saveSIFTBinFile(argv[2], keypioints);
	}
	else {
		LiDARReg::saveSIFTASCIIFile(argv[2], keypioints, g_save_sift_ascii_descriptor);
	}
	return 0;
#endif

//     LiDARReg::saveSIFTBinFile("D:\\Libraries\\Documents\\Project\\Stocker_Test\\Work\\guangzhou\\RegistrationTest\\81550_150560147_034_20160205072930_rgb2.key", keypioints);
// 	return 1;
	//if (argc < 4)
	//{
	//	Usage();
	//}
	////argv[1] = "G:\\==宁波倾斜测试数据==\\Test\\数据\\下视+下视\\126.jpg";
	////argv[2] = "G:\\==宁波倾斜测试数据==\\Test\\数据\\下视+下视\\126.key";
	////argv[3] = "5000";
	//ExtraFeatPt(argv[1], argv[2], atoi(argv[3]));
	//return 0;

	{
		loadIni();
	}

	if (argc == 4)
	{
		ExtraFeatPt(argv[1], argv[2], atoi(argv[3]));
	}
	else if (argc == 2)
	{
		char szParaFPath[_MAX_PATH];
		sprintf(szParaFPath, "%s", argv[1]);
		FILE* fid = fopen(szParaFPath, "r");
		if (!fid)
			return 0;
		char szbuf[1024];
		char szImgPath[_MAX_PATH];
		char szOutFeaPath[_MAX_PATH];
		int nBlockSize = 5000;
		fgets(szbuf, 1024, fid);
		sscanf(szbuf, "%s%s%d", szImgPath, szOutFeaPath, &nBlockSize);
		fclose(fid);
		ExtraFeatPt(szImgPath, szOutFeaPath, nBlockSize);
	}
	return 0;
}

#ifdef ANSI_SIFT_FILE
void ExtraFeatPt(const char *lpImgPath, const char* lpOutFeaPath, int nBlockSize/* = 2500*/)
{
	if (_access(lpOutFeaPath, 0) == 0)
		return;

	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");  //支持中文路径

	GDALDataset *poDataSet = NULL;
	poDataSet = (GDALDataset*)GDALOpen(lpImgPath, GA_ReadOnly);

	if (!poDataSet)
	{
		return;
	}

	int nX = poDataSet->GetRasterXSize();
	int nY = poDataSet->GetRasterYSize();
	int nBands = poDataSet->GetRasterCount();
	GDALDataType dT = poDataSet->GetRasterBand(1)->GetRasterDataType();

	int nw, nh;
	nw = nh = 1;//将影像分成nw*nh个块

	int BlockSize = nBlockSize;

	if (nX > BlockSize && nY > BlockSize)
	{
		nw = nX / BlockSize;
		if (nX % BlockSize != 0)
		{
			nw++;
		}
		nh = nY / BlockSize;
		if (nY % BlockSize != 0)
		{
			nh++;
		}
	}

	if (nX > BlockSize && nY <= BlockSize)
	{
		nw = nX / BlockSize;
		if (nX % BlockSize != 0) {
			nw++;
		}
		nh = 1;
	}

	if (nX <= BlockSize && nY > BlockSize)
	{
		nw = 1;
		nh = nY / BlockSize;
		if (nh % BlockSize != 0) {
			nh++;
		}
	}

	if (nX <= BlockSize && nY <= BlockSize)
	{
		nw = nh = 1;
	}


	FILE *f = fopen(lpOutFeaPath, "w");

	if (!f)
	{
		printf("[ExtraFeature] Cannot Write Feature File: %s\n", lpOutFeaPath);
		return;
	}

	printf("\n-----------------------------------------------------------\n"
		"Image Name: %s\n"
		"Init Image Size: (width)%d * (height)%d\n"
		"Split into: %d * %d\n", lpImgPath, nX, nY, nw, nh);

	int sw, sh, ew, eh, w, h;
	int ptNum, ptSum = 0;

	fprintf(f, "%10d 128\n", ptSum);

	clock_t time_start = clock();

	SiftGPU *sift = new SiftGPU;

#if 0
	char*  para[] = { "-fo", "0", "-v", "1", "-s", "1", "-loweo", "(0.5,0.5)", "-m", "1" };
#endif
	char*  para[] = { "-fo", "0", "-v", "1", "-s", "1", "-loweo", "(0.5,0.5)", "-m", "1" };

	sift->ParseParam(10, para);
	int support = sift->CreateContextGL();

	if (support != SiftGPU::SIFTGPU_FULL_SUPPORTED)
	{
		printf("GL 环境错误");
		return;
	}

	//提取结果
	printf("Start Extracting Sift Features from up to down, left to right: \n");

	for (int i = 0; i < nw; i++)
	{
		for (int j = 0; j < nh; j++)
		{
			printf("Block(%d, %d):\n", i, j);
			sw = i * BlockSize;
			ew = (i + 1) * BlockSize - 1;

			if (ew >= nX - 1)
			{
				ew = nX - 1;
			}

			sh = j * BlockSize;
			eh = (j + 1) * BlockSize - 1;

			if (eh >= nY - 1)
			{
				eh = nY - 1;
			}

			w = ew - sw + 1;//数据宽度
			h = eh - sh + 1;//数据高度

			unsigned char* pData = new unsigned char[w * h];//2015-03-12, ==>>'+128'

			if (nBands == 3)
			{
				unsigned char* pD = new unsigned char[w*h*nBands];
				//int panBandMap[3] = { 1, 2, 3 }; //RGB
				poDataSet->RasterIO(GF_Read, sw, sh, w, h, pD, w, h, GDT_Byte, nBands, 0, 0, 0, 0);

#pragma omp parallel for
				for (int k = 0; k < w*h; k++)
				{//RGB转灰度		
					pData[k] = int((pD[k] + pD[w * h + k] + pD[2 * w * h + k]) / 3 + 0.5);
				}

				delete[] pD;
			}
			else if (nBands == 1)
			{
				GDALRasterBand* pBand = poDataSet->GetRasterBand(nBands);
				pBand->RasterIO(GF_Read, sw, sh, w, h, pData, w, h, GDT_Byte, 0, 0);
			}
			else
			{
				break;
			}


			if (!pData)
			{
				printf("[ExtraFeatPt] Data Error!\n");
				GDALClose(poDataSet);
				return;
			}

			sift->RunSIFT(w, h, pData, GL_LUMINANCE, GL_UNSIGNED_BYTE);

			if (pData)
			{
				delete[] pData;
				pData = NULL;
			}

			ptNum = sift->GetFeatureNum();
			ptSum += ptNum;

			printf("[ExtraFeatPt] Block(%d, %d): Find Features : %d\n", i, j, ptNum);

			vector<SiftGPU::SiftKeypoint> key_temp(ptNum);
			vector<float> des_temp(128 * ptNum);
			sift->GetFeatureVector(&key_temp[0], &des_temp[0]);

			for (int k = 0; k < key_temp.size(); k++)
			{
				key_temp[k].x += i * BlockSize;
				key_temp[k].y += j * BlockSize;

				fprintf(f, "%.2f %.2f %.3f %.3f\n", key_temp[k].y, key_temp[k].x, key_temp[k].s, key_temp[k].o); //行，列

				for (int kd = 0; kd < 128; kd++)
				{
					fprintf(f, "%d ", (unsigned int)floor(0.5 + 512.0f * des_temp[128 * k + kd]));

					if ((kd + 1) % 20 == 0)
					{
						fprintf(f, "\n");
					}
				}

				fprintf(f, "\n");
			}

			key_temp.clear();
			des_temp.clear();

		}//nh - height
	}//nw - width

	if (poDataSet)
		GDALClose(poDataSet);

	long size = ftell(f);

	fseek(f, -size, SEEK_END);
	fprintf(f, "%10d 128\n", ptSum);

	fclose(f);

	sift->DestroyContextGL();

	if (sift) delete  sift;

	clock_t time_end = clock();

	printf("[ExtraFeaturePt] Time using: %.3lf\n", double(time_end - time_start) / CLK_TCK);

}
#else
void ExtraFeatPt(const char *lpImgPath, const char* lpOutFeaPath, int nBlockSize/* = 2500*/)
{
	if (_access(lpOutFeaPath, 0) == 0)
		return;

	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");  //支持中文路径

	GDALDataset *poDataSet = NULL;
	poDataSet = (GDALDataset*)GDALOpen(lpImgPath, GA_ReadOnly);

	if (!poDataSet)
	{
		return;
	}

	int nX = poDataSet->GetRasterXSize();
	int nY = poDataSet->GetRasterYSize();
	int nBands = poDataSet->GetRasterCount();
	GDALDataType dT = poDataSet->GetRasterBand(1)->GetRasterDataType();

	int nw, nh;
	nw = nh = 1;//将影像分成nw*nh个块

	int BlockSize = nBlockSize;

	if (nX > BlockSize && nY > BlockSize)
	{
		nw = nX / BlockSize;
		if (nX % BlockSize != 0)
		{
			nw++;
		}
		nh = nY / BlockSize;
		if (nY % BlockSize != 0)
		{
			nh++;
		}
	}

	if (nX > BlockSize && nY <= BlockSize)
	{
		nw = nX / BlockSize;
		if (nX % BlockSize != 0) {
			nw++;
		}
		nh = 1;
	}

	if (nX <= BlockSize && nY > BlockSize)
	{
		nw = 1;
		nh = nY / BlockSize;
		if (nh % BlockSize != 0) {
			nh++;
		}
	}

	if (nX <= BlockSize && nY <= BlockSize)
	{
		nw = nh = 1;
	}

	printf("\n-----------------------------------------------------------\n"
		"Image Name: %s\n"
		"Init Image Size: (width)%d * (height)%d\n"
		"Split into: %d * %d\n", lpImgPath, nX, nY, nw, nh);

	int sw, sh, ew, eh, w, h;
	int ptNum, ptSum = 0;

//	fprintf(f, "%10d 128\n", ptSum);

	clock_t time_start = clock();

	SiftGPU *sift = new SiftGPU;

#if 0
	char*  para[] = { "-fo", "0", "-v", "1", "-s", "1", "-loweo", "(0.5,0.5)", "-m", "1" };
#endif
	char*  para[] = { "-fo", "0", "-v", "1", "-s", "1", "-loweo", "(0.5,0.5)", "-m", "1" };

	sift->ParseParam(10, para);
	int support = sift->CreateContextGL();

	if (support != SiftGPU::SIFTGPU_FULL_SUPPORTED)
	{
		printf("GL 环境错误");
		return;
	}

	//提取结果
	printf("Start Extracting Sift Features from up to down, left to right: \n");

	std::vector<LiDARReg::SIFT_KEY_POINT> sift_key_points_all;
	for (int i = 0; i < nw; i++)
	{
		for (int j = 0; j < nh; j++)
		{
			printf("Block(%d, %d):\n", i, j);
			sw = i * BlockSize;
			ew = (i + 1) * BlockSize - 1;

			if (ew >= nX - 1)
			{
				ew = nX - 1;
			}

			sh = j * BlockSize;
			eh = (j + 1) * BlockSize - 1;

			if (eh >= nY - 1)
			{
				eh = nY - 1;
			}

			w = ew - sw + 1;//数据宽度
			h = eh - sh + 1;//数据高度

			unsigned char* pData = new unsigned char[w * h];//2015-03-12, ==>>'+128'

			if (nBands == 3)
			{
				unsigned char* pD = new unsigned char[w*h*nBands];
				//int panBandMap[3] = { 1, 2, 3 }; //RGB
				poDataSet->RasterIO(GF_Read, sw, sh, w, h, pD, w, h, GDT_Byte, nBands, 0, 0, 0, 0);

#pragma omp parallel for
				for (int k = 0; k < w*h; k++)
				{//RGB转灰度		
					pData[k] = int((pD[k] + pD[w * h + k] + pD[2 * w * h + k]) / 3 + 0.5);
				}

				delete[] pD;
			}
			else if (nBands == 1)
			{
				GDALRasterBand* pBand = poDataSet->GetRasterBand(nBands);
				pBand->RasterIO(GF_Read, sw, sh, w, h, pData, w, h, GDT_Byte, 0, 0);
			}
			else
			{
				break;
			}


			if (!pData)
			{
				printf("[ExtraFeatPt] Data Error!\n");
				GDALClose(poDataSet);
				return;
			}

			sift->RunSIFT(w, h, pData, GL_LUMINANCE, GL_UNSIGNED_BYTE);

			if (pData)
			{
				delete[] pData;
				pData = NULL;
			}

			ptNum = sift->GetFeatureNum();
			ptSum += ptNum;

			printf("[ExtraFeatPt] Block(%d, %d): Find Features : %d\n", i, j, ptNum);

			vector<float> key_temp(4 * ptNum);
			vector<float> des_temp(128 * ptNum);
			sift->GetFeatureVector((SiftGPU::SiftKeypoint*)&key_temp[0], &des_temp[0]);
			std::vector<LiDARReg::SIFT_KEY_POINT> sift_key_points;
			try {
				sift_key_points.resize(ptNum);
			}
			catch (const std::bad_alloc& e) {
				std::cerr << e.what() << std::endl;
				return;
			}

#pragma omp parallel for
			for (int k = 0; k < ptNum; k++)
			{
				LiDARReg::SIFT_KEY_POINT& key_point = sift_key_points[k];
				key_point.keys[0] = key_temp[4 * k + 0] + i * BlockSize; // x
				key_point.keys[1] = key_temp[4 * k + 1] + j * BlockSize; // y
				key_point.keys[2] = key_temp[4 * k + 2];
				key_point.keys[3] = key_temp[4 * k + 3];

				for (int kd = 0; kd < 128; kd++) {
					key_point.descriptors[kd] = (unsigned char)floor(0.5 + 512.0f * des_temp[128 * k + kd]);
				}
			}

			key_temp.clear();
			des_temp.clear();

			sift_key_points_all.insert(sift_key_points_all.end(), sift_key_points.begin(), sift_key_points.end());

		}//nh - height
	}//nw - width

	if (g_sift_point_kept > 0 && sift_key_points_all.size() > g_sift_point_kept) {
		PARALLEL_SORT(sift_key_points_all.begin(), sift_key_points_all.end(), [&](auto _l, auto _r) {
			return _l.keys[2] > _r.keys[2];
		});
		sift_key_points_all.resize(g_sift_point_kept);
	}

	if (g_save_sift_as_bin) {
		if (!saveSIFTBinFile(lpOutFeaPath, sift_key_points_all)) {
			std::cerr << "failed to save sift file: " << lpOutFeaPath << std::endl;
		}
	}
	else {
		if (!saveSIFTASCIIFile(lpOutFeaPath, sift_key_points_all, g_save_sift_ascii_descriptor)) {
			std::cerr << "failed to save sift file: " << lpOutFeaPath << std::endl;
		}
	}
	

	if (poDataSet)
		GDALClose(poDataSet);

	sift->DestroyContextGL();

	if (sift) delete  sift;

	clock_t time_end = clock();

	printf("[ExtraFeaturePt] Time using: %.3lf\n", double(time_end - time_start) / CLK_TCK);

}
#endif // ANSI_SIFT_FILE