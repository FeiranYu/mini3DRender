#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <tchar.h>
#include<atlimage.h>
#include<time.h>


using namespace std;

typedef unsigned int IUINT32;

//=====================================================================
// 数学库：此部分应该不用详解，熟悉 D3D 矩阵变换即可
//=====================================================================
typedef struct { float m[4][4]; } matrix_t;
typedef struct Vector_t { float x, y, z, w;
Vector_t(float x, float y, float z)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = 1.0f;
}
Vector_t(float x, float y, float z,float w)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}
Vector_t()
{

}

} vector_t;
typedef vector_t point_t;

typedef struct { float r, g, b; } color_t;
typedef struct { float u, v; } texcoord_t;
typedef struct { point_t pos; texcoord_t tc; vector_t vn; color_t color; float rhw; } vertex_t;

typedef struct { vertex_t v, v1, v2; } edge_t;
typedef struct { float top, bottom; edge_t left, right; } trapezoid_t;
typedef struct { vertex_t v, step; int x, y, w; } scanline_t;

int CMID(int x, int min, int max);

// 计算插值：t 为 [0, 1] 之间的数值
inline float interp(float x1, float x2, float t);

// | v |
float vector_length(const vector_t* v);


void vector_scale(vector_t* v1, vector_t* v0, float size);

// z = x + y
void vector_add(vector_t* z, const vector_t* x, const vector_t* y);

// z = x - y
void vector_sub(vector_t* z, const vector_t* x, const vector_t* y);

// 矢量点乘
float vector_dotproduct(const vector_t* x, const vector_t* y);

// 矢量叉乘
void vector_crossproduct(vector_t* z, const vector_t* x, const vector_t* y);

// 矢量插值，t取值 [0, 1]
void vector_interp(vector_t* z, const vector_t* x1, const vector_t* x2, float t);

// 矢量归一化
void vector_normalize(vector_t* v);

//让v_in向量绕m_in旋转fAngle个角度(只计算三维) 和matrix_apply 只差不计算第四维
void vector_TransformCoord(vector_t* VOut, vector_t* vIn, matrix_t* M);

// c = a + b
void matrix_add(matrix_t* c, const matrix_t* a, const matrix_t* b);

// c = a - b
void matrix_sub(matrix_t* c, const matrix_t* a, const matrix_t* b);

// c = a * b
void matrix_mul(matrix_t* c, const matrix_t* a, const matrix_t* b);

// c = a * f
void matrix_scale(matrix_t* c, const matrix_t* a, float f);

// y = x * m
void matrix_apply(vector_t* y, const vector_t* x, const matrix_t* m);

void matrix_set_identity(matrix_t* m);

void matrix_set_zero(matrix_t* m);

// 平移变换
void matrix_set_translate(matrix_t* m, float x, float y, float z);

// 缩放变换
void matrix_set_scale(matrix_t* m, float x, float y, float z);

// 旋转矩阵
void matrix_set_rotate(matrix_t* m, float x, float y, float z, float theta);


struct TriangleIndex_t {
	int pointA, pointB, pointC;
	int uvA, uvB, uvC;
	int vnA, vnB, vnC;
};



struct Object {
	vector_t* point;
	texcoord_t* uv;
	vector_t* vn;
	TriangleIndex_t* triangleIndex;
	int pointSum;
	int uvSum;
	int vnSum;
	int TriangleIndexSum;
	IUINT32* texture;
	int tex_width;
	int tex_height;
	vector_t pos;
};




Object LoadObject(const char* filename);