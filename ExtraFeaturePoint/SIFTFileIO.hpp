
#ifndef __SIFT_File_IO__
#define __SIFT_File_IO__
//#endif
#ifndef LIDARREG_NAMESPACE_BEGIN
#define LIDARREG_NAMESPACE_BEGIN namespace LiDARReg{
#endif
#ifndef LIDARREG_NAMESPACE_END
#define LIDARREG_NAMESPACE_END }
#endif

#include <vector>
#include <iostream>
#include <sys/io.h>
LIDARREG_NAMESPACE_BEGIN

#define KEYFEATURE_NUM 4
#define DESCRIPTOR_NUM 128

struct SIFT_KEY_POINT {
	SIFT_KEY_POINT() {
		memset(keys, 0, KEYFEATURE_NUM*sizeof(float));
		memset(descriptors, 0, DESCRIPTOR_NUM*sizeof(unsigned char));
	}
	float keys[KEYFEATURE_NUM]; // x y scale orientation
	unsigned char descriptors[DESCRIPTOR_NUM];
};

inline bool loadSIFTBinFile(const char * path, std::vector<SIFT_KEY_POINT> & sift_key_points) 
{
	FILE *fp = fopen(path, "rb");
	if (!fp) {
		std::cerr << "failed to open file: " << path << std::endl;
		return false;
	}

	int pt_num(0), des_num(0);
	if (fread(&pt_num, sizeof(int), 1, fp) == 0) return false;
	if (fread(&des_num, sizeof(int), 1, fp) == 0) return false;
	if (des_num != DESCRIPTOR_NUM) {
		std::cerr << "wrong descriptor num: " << des_num << std::endl;
		return false;
	}
	try {
		sift_key_points.resize(pt_num);
	}
	catch (const std::bad_alloc& e) {
		std::cerr << e.what() << std::endl;
		return false;
	}
	fread(&sift_key_points[0], sizeof(SIFT_KEY_POINT), pt_num, fp);

	fclose(fp);
	return true;
}

inline bool saveSIFTBinFile(const char * path, const std::vector<SIFT_KEY_POINT> & sift_key_points) 
{
	FILE *fp = fopen(path, "wb");
	if (!fp) {
		std::cerr << "failed to open file: " << path << std::endl;
		return false;
	}
	int pt_num = sift_key_points.size();
	int des_num = DESCRIPTOR_NUM;
	if (fwrite(&pt_num, sizeof(int), 1, fp) == 0) return false;
	if (fwrite(&des_num, sizeof(int), 1, fp) == 0) return false;
	fwrite(&sift_key_points[0], sizeof(SIFT_KEY_POINT), pt_num, fp);

	fclose(fp);
	return true;
}

inline bool loadSIFTASCIIFile(const char * path, std::vector<SIFT_KEY_POINT> & sift_key_points)
{
	//windows下使用fopen_s
	//FILE *fp = nullptr;  
	//fopen_s(&fp, path, "r");
	FILE *fp=fopen(path,"r");
	if (!fp) {
		std::cerr << "failed to open file: " << path << std::endl;
		return false;
	}

	int pt_num(0), des_num(0);
	fscanf(fp, "%d%d", &pt_num, &des_num);
	if (des_num != DESCRIPTOR_NUM) {
		std::cerr << "wrong descriptor num: " << des_num << std::endl;
		return false;
	}

	try {
		sift_key_points.resize(pt_num);
	}
	catch (const std::bad_alloc& e) {
		std::cerr << e.what() << std::endl;
		return false;
	}

	for (size_t i = 0; i < pt_num; i++)
	{
		auto & key_pt = sift_key_points[i];
		fscanf(fp, "%f%f%f%f", &key_pt.keys[0], &key_pt.keys[1], &key_pt.keys[2], &key_pt.keys[3]);
		for (size_t j = 0; j < des_num; j++) {
			fscanf(fp, "%d", &key_pt.descriptors[j]);
		}
	}

	fclose(fp);
	return true;
}

inline bool saveSIFTASCIIFile(const char * path, const std::vector<SIFT_KEY_POINT> & sift_key_points, bool save_descriptor)
{
	//FILE *fp = nullptr;  
	//fopen_s(&fp, path, "w");
	FILE *fp=fopen(path,"w");
	if (!fp) {
		std::cerr << "failed to open file: " << path << std::endl;
		return false;
	}
	int pt_num = sift_key_points.size();
	int des_num = DESCRIPTOR_NUM;

	fprintf(fp, "%d %d\n", sift_key_points.size(), 128);
	for (size_t i = 0; i < sift_key_points.size(); i++) {
		double x = sift_key_points[i].keys[0];
		double y = sift_key_points[i].keys[1];
		double s = sift_key_points[i].keys[2];
		double o = sift_key_points[i].keys[3];

		fprintf(fp, "%.2f %.2f %.3f %.3f\n", y, x, s, o);

		if (save_descriptor) {
			for (int kd = 0; kd < 128; kd++)
			{
				fprintf(fp, "%d ", sift_key_points[i].descriptors[kd]);

				if ((kd + 1) % 20 == 0)
				{
					fprintf(fp, "\n");
				}
			}
			fprintf(fp, "\n");
		}
	}
	fclose(fp);
	return true;
}


LIDARREG_NAMESPACE_END
#endif