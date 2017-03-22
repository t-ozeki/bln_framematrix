// blueninja_c.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <stdio.h>
#include <math.h>

#define GRV 9.80665
#define TIME 100
#define PI 3.141592

int Transform_Raw(int val);
void FrameMatrix(int time,int ax, int ay, int az, int gx, int gy, int gz, int *cax, int *cay, int *caz);
void EraseOffset(int *gx, int *gy,int *gz);

int main()
{
	FILE *fp,*fwp;
	char *fname = "teibou_ouro.csv";
	char *fwname = "teibou_ouro_calc.csv";
	char s[100];
	int ms[2],ret;
	int a[3], b_a[3], fm_a[3] = {0,0,0};
	float av[3] = {0,0,0};
	int g[3],b_g[3];
	int count = 0;


	fp = fopen(fname, "r");
	if (fp == NULL) {
		printf("%sファイルが開けません¥n", fname);
		return -1;
	}
	fwp = fopen(fwname, "w");

	//一行目
	fscanf(fp, "%s",s);
	fprintf(fwp, "ms,ax,ay,az,gx,gy,gz,fm_ax,fm_ay,fm_az,vx(km/h),vy(km/h),vz(km/h)\n");


	//初期値設定 二行目
	fscanf(fp, "%d,%d,%d,%d,%d,%d,%d", &ms[0], &a[0], &a[1], &a[2], &g[0], &g[1], &g[2]);
	ms[1] = ms[0];
	//符号付変換
	for (int i = 0; i < 3; i++) {
		a[i] = Transform_Raw(a[i]);
		g[i] = Transform_Raw(g[i]);
		b_a[i] = a[i];
//		b_g[i] = g[i];
	}
	FrameMatrix(100,a[0], a[1], a[2], g[0], g[1], g[2],&fm_a[0], &fm_a[1], &fm_a[2]);

	//速度変換 生値のm/s^2変換で/2048*GRV、m/s -> km/h変換で*3.6
	for (int i = 0; i < 3; i++) {
		av[i] += ((float)fm_a[i]/2048*GRV) * TIME*0.001 *3.6;
	}
	//             1  2  3  4  5  6  7  8  9 10 11 12   13   14
	fprintf(fwp, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.2f,%.2f,%.2f\n",
		ms[0],
		a[0],a[1],a[2],
		g[0],g[1],g[2],
		fm_a[0],fm_a[1],fm_a[2],
		av[0], av[1], av[2]);

	//*****************

	while ((ret = fscanf(fp, "%d,%d,%d,%d,%d,%d,%d", &ms[0], &a[0], &a[1], &a[2], &g[0], &g[1], &g[2] )) != EOF) {
		count++;

		//符号付変換
		for (int i = 0; i < 3; i++) {
			a[i] = Transform_Raw(a[i]);
			g[i] = Transform_Raw(g[i]);
		}

		//LPF
		for (int i = 0; i < 3; i++) {
			a[i] = 0.9 * (float)b_a[i] + 0.1 * (float)a[i];
//			g[i] = 0.9 * (float)b_g[i] + 0.1 * (float)g[i];
			b_a[i] = a[i];
//			b_g[i] = g[i];
		}
		FrameMatrix(ms[0]-ms[1],a[0], a[1], a[2], g[0], g[1], g[2], &fm_a[0], &fm_a[1], &fm_a[2]);

		//速度変換 生値のm/s^2変換で/2048*GRV、m/s -> km/h変換で*3.6
		for (int i = 0; i < 3; i++) {
			av[i] += ((float)fm_a[i] / 2048 * GRV) * TIME * 0.001 *3.6;
		}

		ms[1] = ms[0];
		fprintf(fwp, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.2f,%.2f,%.2f\n",
			ms[0],
			a[0], a[1], a[2],
			g[0], g[1], g[2],
			fm_a[0], fm_a[1], fm_a[2],
			av[0], av[1], av[2]);

	}

	fclose(fp);
	fclose(fwp);


	printf("正常終了\n");
	getchar();

	return 0;
}

int Transform_Raw(int val) {
	if (val < 32768) {
		return val;
	}
	else {
		return -1*(65536 - val);
	}
}

	
void EraseOffset(int *gx, int *gy, int *gz) {
//	*ax -= -12;
//	*ay -= 98;
//	*az -= 1997;
	*gx -= -20;
	*gy -= -3;
	*gz -= 3;
}


//http://www.rugbysensor.com/rugbySystem3.html

void FrameMatrix(int time,int ax, int ay, int az, int gx, int gy, int gz, int *cax,int *cay,int *caz) {
	
	int a[3] = {ax,ay,az};
                  	//生値からrad変換
	float g[3] = {(float)gx/16.4*PI/180, (float)gy/16.4*PI/180, (float)gz/16.4*PI/180};

	// 0-2:各軸の回転角度  3:回転角度
	float theta[4];

	//回転行列Rωθ
	float rot[3][3];
	//静止時のFM行列 E0 = [i,j,k];
	float Ezero[3][3];
	//R*E0
	float re[3][3];

	//単位ベクトルλμν、北からの角度γ、微小単位時間dt
	float lambda, mew, nyu, dt;


	//回転ベクトルの大きさ
	float omega;
	omega = sqrt(g[0] * g[0] + g[1] * g[1] + g[2] * g[2]);

	//単位時間dtの算出
	dt = 0.001*time;

	//回転角度theta
	//重力加速度成分
	theta[0] = asin(-12 / 2048);
	theta[1] = asin( 98 / 2048);
	theta[2] = asin(1997/ 2048);
	theta[3] = omega * dt;

	//［λμν］＝　ω' / ｜ω'｜　　
	if (omega == 0) {
		*cax = a[0];
		*cay = a[1];
		*caz = a[2];
		return;
	}
	else {
		lambda = g[0] / omega;
		mew = g[1] / omega;
		nyu = g[2] / omega;
	}


	//θ=Arccos｛X/(Mcosφ)｝
	//φ=Arcsin（Z/M)
	//M =sqrt(X^2+Y^2+Z^2)

	//回転行列Rωθ
	rot[0][0] = cos(theta[3]) + nyu * nyu * (1 - cos(theta[3]));
	rot[0][1] = nyu * mew * (1 - cos(theta[3])) - nyu * sin(theta[3]);
	rot[0][2] = mew * sin(theta[3]) + lambda * nyu * (1 - cos(theta[3]));
	rot[1][0] = nyu * sin(theta[3]) + mew * lambda * (1 - cos(theta[3]));
	rot[1][1] = cos(theta[3]) + mew * mew * (1 - cos(theta[3]));
	rot[1][2] = mew * nyu * (1 - cos(theta[3])) - lambda * sin(theta[3]);
	rot[2][0] = nyu * lambda * (1 - cos(theta[3])) - mew * sin(theta[3]);
	rot[2][1] = lambda * sin(theta[3]) + nyu * mew * (1 - cos(theta[3]));
	rot[2][2] = cos(theta[3]) + nyu * nyu * (1 - cos(theta[3]));

	//E0の算出
	Ezero[0][0] = cos(theta[0]);
	Ezero[1][0] = 0;
	Ezero[2][0] = sin(theta[0]);
	Ezero[0][1] = -1 * tan(theta[0]) * sin(theta[1]);
	Ezero[1][1] = sin(theta[2]) / cos(theta[0]);
	Ezero[2][1] = sin(theta[1]);
	Ezero[0][2] = -1 * tan(theta[0]) * sin(theta[2]);
	Ezero[1][2] = -1 * sin(theta[1]) / cos(theta[0]);
	Ezero[2][2] = sin(theta[2]);

	// 行列の積RE
	for (int i = 0; i<3; i++) {
		for (int j = 0; j<3; j++) {
			re[i][j] = rot[i][0] * Ezero[0][j] + rot[i][1] * Ezero[1][j] + rot[i][2] * Ezero[2][j];
		}
	}

	*cax = re[0][0] * a[0] + re[0][1] * a[1] + re[0][2] * a[2];
	*cay = re[1][0] * a[0] + re[1][1] * a[1] + re[1][2] * a[2];
	*caz = re[2][0] * a[0] + re[2][1] * a[1] + re[2][2] * a[2];

	if (*cax < -2100000000) {
		printf("%f,%f,%f,%f\n",nyu, re[0][0], re[0][1], re[0][2]);
	}

}
