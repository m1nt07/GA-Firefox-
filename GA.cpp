#include <iostream> 
#include <stdlib.h> 
#include <time.h> 
#include <algorithm>
#include <string>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;
using namespace std;

#define POP_NUM      16						//种群Population大小即扇贝的个数
#define TRIANGLES_NUM      100		//每个扇贝上三角形个数
#define INTERVAL		  20						//绘制种群的频率
#define ITER_NUM     300000				//迭代次数(iteration number)
#define CROSS_RATE	 0.86					//交叉率(crossover rate)取值范围一般为0.6 ~ 1
#define MUT_RATE     0.1					//变异率(mutation rate)取值范围一般为0.0001～0.1

typedef struct Triangle {
	Point point1;
	Point point2;
	Point point3;
	Scalar color;
};
typedef struct Scallop {
	Triangle chromosome[TRIANGLES_NUM];
	double fit_value;
	bool update_flag;//代表该扇贝在当代繁衍过程中是否被改变
};

Mat ideal_image;								//想要得到的图像
int ideal_image_length;					//想要得到的图像的大小


//绘制扇贝
void Draw_Scallop(Scallop &scallop, Mat &dst_image)
{
	Mat new_temp_image(ideal_image_length, ideal_image_length, CV_8UC4);

	for (int i_triangles = 0; i_triangles < TRIANGLES_NUM; i_triangles++)
	{
		//初始化
		Mat_<Vec4b>::iterator begin, end;
		begin = new_temp_image.begin<Vec4b>(); end = new_temp_image.end<Vec4b>();
		while (begin != end)
		{
			(*begin)[0] = 0;	(*begin)[1] = 0;	(*begin)[2] = 0;	(*begin)[3] = 0;
			begin++;
		}

		//三角形叠加至目标图像
		Point contour1[3] = { scallop.chromosome[i_triangles].point1,scallop.chromosome[i_triangles].point2,scallop.chromosome[i_triangles].point3 };
		const Point* pts[1] = { contour1 };
		int npt1[] = { 3 };
		//检测
		fillPoly(new_temp_image, pts, npt1, 1, scallop.chromosome[i_triangles].color, 8);

		for (int i = 0; i < new_temp_image.rows; i++)
		{
			for (int j = 0; j < new_temp_image.cols; j++)
			{
				Vec4b& new_point = new_temp_image.at<Vec4b>(i, j);
				Vec4b& dst_point = dst_image.at<Vec4b>(i, j);
				if (new_point[0] == 0 && new_point[1] == 0 && new_point[2] == 0 && new_point[3] == 0) {}
				else
				{
					//颜色通道 C=C0*(1-alpha1)+C1*alpha1 （0代表新建图层alpha，1代表背景图层alpha）
					//透明通道 alpha = 1 - ( 1 - alpah1 ) * ( 1 - alpha2 ) = alpha1 + alpha2 - alpha1 * alpha2
					double alpha0 = (double)dst_point[3] / (double)255;
					double alpha1 = (double)new_point[3] / (double)255;
					dst_point[0] = int(dst_point[0] * (1 - alpha1) + new_point[0] * alpha1);
					dst_point[1] = int(dst_point[1] * (1 - alpha1) + new_point[1] * alpha1);
					dst_point[2] = int(dst_point[2] * (1 - alpha1) + new_point[2] * alpha1);
					dst_point[3] = int(255 * (alpha0 + alpha1 - alpha0*alpha1));
				}
			}
		}
	}
}


//计算染色体的适应度值
double Calculate_Fit_Value(Scallop &scallop)
{
	Mat dst_image(ideal_image_length, ideal_image_length, CV_8UC4, Scalar(0, 0, 0, 0));
	double fit_result = 0.0;

	//绘制该扇贝
	Draw_Scallop(scallop, dst_image);

	//计算该种群形成的图像内部所有像素点与想得到图像像素点的差值和
	for (int i = 0; i < dst_image.rows; i++)
	{
		for (int j = 0; j < dst_image.cols; j++)
		{
			Vec4b& ideal_point = ideal_image.at<Vec4b>(i, j);
			Vec4b& dst_point = dst_image.at<Vec4b>(i, j);
			fit_result += abs(ideal_point[0] - dst_point[0]) + abs(ideal_point[1] - dst_point[1]) + abs(ideal_point[2] - dst_point[2]) + abs(ideal_point[3] - dst_point[3]);
		}
	}

	//差值和除以像素点数目的倒数作为适应度
	//return 1.0 / (fit_result / (double)(ideal_image_length*ideal_image_length*4));
	return (double)(ideal_image_length*ideal_image_length * 4) / fit_result;
}


//创建初始群体                               
void Rand_Create_Pop(Scallop* pop)
{
	// 遍历种群中所有染色体
	for (int i = 0; i<POP_NUM; i++)
	{
		// 基因初始化
		for (int j = 0; j < TRIANGLES_NUM; j++)
		{
			pop[i].chromosome[j].point1 = Point(rand() % ideal_image_length, rand() % ideal_image_length); 
			pop[i].chromosome[j].point2 = Point(rand() % ideal_image_length, rand() % ideal_image_length); 
			pop[i].chromosome[j].point3 = Point(rand() % ideal_image_length, rand() % ideal_image_length); 
			pop[i].chromosome[j].color = Scalar(rand() % 255, rand() % 255, rand() % 255, rand() % 255);
		}

		// 计算染色体的适应度
		pop[i].fit_value = Calculate_Fit_Value(pop[i]);

		//更新标志置为false
		pop[i].update_flag = false;
	}
}


//选择(Selection):根据适应度选择优良个体
int Selected_Gene_Pos(double* accProb)
{
	//轮盘赌算法寻找优秀个体
	double rand_double = double(rand() % 1000) / (double)1000;

	for (int i = 0; i < POP_NUM; i++)
	{
		if (rand_double < accProb[i])
			return i;
	}
}

void Selection(Scallop *pop)
{
	pair<int, double> min_scallop = pair<int, double>(0, pop[0].fit_value);
	pair<int, double> second_min_scallop = pair<int, double>(0, pop[0].fit_value);
	int pop_num_1, pop_num_2;
	
	//找到当代种群适应度最小与次最小扇贝
	for (int i = 0; i < POP_NUM; i++)
		min_scallop = (min_scallop.second <= pop[i].fit_value) ? min_scallop : pair<int, double>(i, pop[i].fit_value);
	pop[min_scallop.first].fit_value = 0.0;
	for (int i = 0; i < POP_NUM; i++)
	{
		if(pop[i].fit_value==0.0){}
		else
			second_min_scallop = (second_min_scallop.second <= pop[i].fit_value) ? second_min_scallop : pair<int, double>(i, pop[i].fit_value);
	}

	//除最小次最小外随机选择两个体交叉产生两新个体
	do {
		pop_num_1 = rand() % POP_NUM;
	} while (pop_num_1 == min_scallop.first||pop_num_1 == second_min_scallop.first);
	do {
		pop_num_2 = rand() % POP_NUM;
	} while (pop_num_2 == pop_num_1||pop_num_2 == min_scallop.first||pop_num_2 == second_min_scallop.first);
	for (int j = 0; j < TRIANGLES_NUM; j++)
	{
		if (rand() % 2 == 0)
		{
			pop[min_scallop.first].chromosome[j] = pop[pop_num_1].chromosome[j];
			pop[second_min_scallop.first].chromosome[j] = pop[pop_num_2].chromosome[j];
		}
		else
		{
			pop[second_min_scallop.first].chromosome[j] = pop[pop_num_1].chromosome[j];
			pop[min_scallop.first].chromosome[j] = pop[pop_num_2].chromosome[j];
		}
	}

	//更新适应度
	pop[min_scallop.first].fit_value = Calculate_Fit_Value(pop[min_scallop.first]);
	pop[second_min_scallop.first].fit_value = Calculate_Fit_Value(pop[second_min_scallop.first]);

	//更新标志置为true
	pop[min_scallop.first].update_flag = true;
	pop[second_min_scallop.first].update_flag = true;


	//double sum_fit_value = 0.0,  accProbability[POP_NUM];
	//Scallop temp_pop[POP_NUM];
	//pair<int, double> temp_min_scallop = pair<int, double>(0, pop[0].fit_value);
	//
	////将当代种群适应度最小扇贝适应度置为0，淘汰
	//for (int i = 0; i < POP_NUM; i++)
	//	temp_min_scallop = (temp_min_scallop.second <= pop[i].fit_value) ? temp_min_scallop : pair<int, double>(i, pop[i].fit_value);
	//pop[temp_min_scallop.first].fit_value = 0.0;
	//	
	////总适应度
	//for (int i = 0; i < POP_NUM; i++)
	//	sum_fit_value += pop[i].fit_value;

	////累积概率
	//accProbability[0] = pop[0].fit_value / sum_fit_value;
	//for (int i = 1; i < POP_NUM; i++)
	//	accProbability[i] = pop[i].fit_value / sum_fit_value + accProbability[i - 1];
	//
	////轮盘赌算法寻找优秀个体
	//for (int i = 0; i < POP_NUM; i++)
	//	temp_pop[i] = pop[Selected_Gene_Pos(accProbability)];
	//for (int i = 0; i < POP_NUM; i++)
	//	pop[i] = temp_pop[i];
}


 //交叉(Crossover):扇贝随机两两配对交换基因
void Crossover(Scallop *pop)
{
	//未选择过的下标
	vector<int> unselected_pos;
	vector<int>::iterator iter;
	int pop_num, gene_begin, gene_end;

	//记录未选择过的元素下标，初始化为0,1,2.....POP_NUM
	for (int i = 0; i < POP_NUM; i++)
		unselected_pos.push_back(i);

	for (int i = 0; i < POP_NUM; i++)
	{
		if (count(unselected_pos.begin(), unselected_pos.end(), i))
		{
			//删除该下标
			iter = find(unselected_pos.begin(), unselected_pos.end(), i);
			unselected_pos.erase(iter);

			//随机寻找与之配对的染色体并删除配对的下标
			pop_num = unselected_pos[rand() % unselected_pos.size()];
			iter = find(unselected_pos.begin(), unselected_pos.end(), pop_num);
			unselected_pos.erase(iter);

			//有CROSS_RATE概率交叉
			if (rand() % 100 <= int(CROSS_RATE * 100))
			{
				Scallop temp_scallop = pop[i];
				for (int j = 0; j < TRIANGLES_NUM; j++)
				{
					if (rand() % 2 == 0)
					{
						pop[i].chromosome[j] = pop[pop_num].chromosome[j];
						pop[pop_num].chromosome[j] = temp_scallop.chromosome[j];
					}
				}

				//更新标志置为true
				pop[i].update_flag = true;
				pop[pop_num].update_flag = true;
			}
		}
	}
}


 //变异(Mutation) :随机改变染色体基因的值
void Mutation(Scallop *pop)
{
	int gene_num_1, gene_num_2;

	for (int i = 0; i < POP_NUM; i++)
	{
		//有MUT_RATE概率变异
		if (rand() % 100 <= int(MUT_RATE * 100))
		{
			//染色体上哪两个基因产生变异
			gene_num_1 = rand() % TRIANGLES_NUM;
			//do {
			//	gene_num_2 = rand() % TRIANGLES_NUM;
			//} while (gene_num_2 == gene_num_1);

			//颜色、位置变异
			pop[i].chromosome[gene_num_1].color = Scalar(rand() % 255, rand() % 255, rand() % 255, 255);
			pop[i].chromosome[gene_num_1].point1 = Point(rand() % ideal_image_length, rand() % ideal_image_length);
			pop[i].chromosome[gene_num_1].point2 = Point(rand() % ideal_image_length, rand() % ideal_image_length);
			pop[i].chromosome[gene_num_1].point3 = Point(rand() % ideal_image_length, rand() % ideal_image_length);

			//pop[i].chromosome[gene_num_2].color = Scalar(rand() % 255, rand() % 255, rand() % 255, 255);
			//pop[i].chromosome[gene_num_2].point1 = Point(rand() % ideal_image_length, rand() % ideal_image_length);
			//pop[i].chromosome[gene_num_2].point2 = Point(rand() % ideal_image_length, rand() % ideal_image_length);
			//pop[i].chromosome[gene_num_2].point3 = Point(rand() % ideal_image_length, rand() % ideal_image_length);
			
			//更新标志置为true
			pop[i].update_flag = true;
		}
	}
}


//更新种群适应度
void Update_Fit_Value(Scallop *pop)
{
	for (int i = 0; i < POP_NUM; i++)
	{
		if(pop[i].update_flag)
			pop[i].fit_value = Calculate_Fit_Value(pop[i]);
	}
}


//输出种群适应度信息
void Print_Pop(Scallop *pop)
{
	for (int i = 0; i < POP_NUM; i++)
	{
		if(pop[i].update_flag)
			printf("%d: fit_value=%f updated\n", i, pop[i].fit_value);
		else
			printf("%d: fit_value=%f\n", i, pop[i].fit_value);
	}
}


//绘制当代种群适应度最高的扇贝
void Draw_Max_Scallop(Scallop *pop, int generation)
{
	//寻找当代种群适应度最大扇贝
	pair<int, double> max_scallop = pair<int, double>(0, pop[0].fit_value);
	for (int i = 0; i < POP_NUM; i++)
		max_scallop = (max_scallop.second >= pop[i].fit_value) ? max_scallop : pair<int, double>(i, pop[i].fit_value);

	//绘制该扇贝
	Mat dst_image(ideal_image_length, ideal_image_length, CV_8UC4, Scalar(0, 0, 0, 0));
	Draw_Scallop(pop[max_scallop.first], dst_image);

	//图片写入文件夹
	vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
	compression_params.push_back(9);//从0-9较高的值意味着更小的尺寸和更长的压缩时间而默认值是3.
	string fitness_str = to_string(pop[max_scallop.first].fit_value), generation_str = to_string(generation);
	string str = "C:\\Visual Studio 2017 Projects\\GA\\GA\\第" + generation_str + "代最优扇贝"+ fitness_str +".png";
	imwrite(str, dst_image, compression_params);
}


int main(int argc, char* argv[])
{
	srand((unsigned)time(NULL));

	Scallop cur_pop[POP_NUM];
	ideal_image = imread("ideal_firefox.png", CV_LOAD_IMAGE_UNCHANGED);
	ideal_image_length = ideal_image.rows;

	// 随机创建初始群体
	printf("随机创建初始群体:\n");
	Rand_Create_Pop(cur_pop);
	for (int i = 0; i < POP_NUM; i++)
		printf("%d: fit_value=%f\n", i, cur_pop[i].fit_value);

	// 种群繁衍
	for (int count = 1; count <= ITER_NUM; count++)
	{
		//更新标志初始化
		for (int i = 0; i < POP_NUM; i++)
			cur_pop[i].update_flag = false;

		// 交叉，变异得到新个体并更新适应度，挑选优秀个体
		Crossover(cur_pop);
		Mutation(cur_pop);
		Update_Fit_Value(cur_pop);
		Selection(cur_pop);

		// 输出当代种群
		printf("种群繁衍至第%d代:\n", count);
		Print_Pop(cur_pop);

		//每隔INTERVAL代绘制当代种群适应度最高的扇贝
		if (count==1||count%INTERVAL == 0)
			Draw_Max_Scallop(cur_pop, count);
	}

	return 0;
}
