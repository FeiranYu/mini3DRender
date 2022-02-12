//=====================================================================
// 
// mini3d.c - Mini Software Render All-In-One
//
// build:
//   mingw: gcc -O3 mini3d.c -o mini3d.exe -lgdi32
//   msvc:  cl -O2 -nologo mini3d.c
//
// history:
//   2007.7.01  skywind  create this file as a tutorial
//   2007.7.02  skywind  implementate texture and color render
//   2008.3.15  skywind  fixed a trapezoid issue
//   2015.8.09  skywind  rewrite with more comment
//   2015.8.12  skywind  adjust interfaces for clearity 
// 
//=====================================================================



#include"main.h"
#include"camera.h"

Camera* pCamera;

float model_angle = 0;
bool first_tri = true;

vector_t light_soure = vector_t(0.0, 100.0, 0.0);
vector_t light_soure_camera;
float light_angle_x = 0;
float light_angle_y = 0;

void show_vector(vector_t& vec)
{
	cout << "vec x " << vec.x << "y " << vec.y << "z " << vec.z << endl;
}


// D3DXMatrixPerspectiveFovLH
void matrix_set_perspective(matrix_t* m, float fovy, float aspect, float zn, float zf) {
	float fax = 1.0f / (float)tan(fovy * 0.5f);
	matrix_set_zero(m);
	m->m[0][0] = (float)(fax / aspect);
	m->m[1][1] = (float)(fax);
	m->m[2][2] = zf / (zf - zn);
	m->m[3][2] = -zn * zf / (zf - zn);
	m->m[2][3] = 1;
}


//=====================================================================
// 坐标变换
//=====================================================================
typedef struct {
	matrix_t world;         // 世界坐标变换
	matrix_t view;          // 摄影机坐标变换
	matrix_t projection;    // 投影变换
	matrix_t transform;     // transform = world * view * projection
	float w, h;             // 屏幕大小
}	transform_t;


// 矩阵更新，计算 transform = world * view * projection
void transform_update(transform_t* ts) {
	matrix_t m;
	matrix_mul(&m, &ts->world, &ts->view);
	matrix_mul(&ts->transform, &m, &ts->projection);
}

// 初始化，设置屏幕长宽
void transform_init(transform_t* ts, int width, int height) {
	float aspect = (float)width / ((float)height);
	matrix_set_identity(&ts->world);
	matrix_set_identity(&ts->view);
	matrix_set_perspective(&ts->projection, 3.1415926f * 0.5f, aspect, 1.0f, 500.0f);
	ts->w = (float)width;
	ts->h = (float)height;
	transform_update(ts);
}

// 将矢量 x 进行 project 
void transform_apply(const transform_t* ts, vector_t* y, const vector_t* x) {
	matrix_apply(y, x, &ts->transform);
}

// 检查齐次坐标同 cvv 的边界用于视锥裁剪
int transform_check_cvv(const vector_t* v) {
	float w = v->w;
	int check = 0;
	if (v->z < 0.0f) check |= 1;
	if (v->z > w) check |= 2;
	if (v->x < -w) check |= 4;
	if (v->x > w) check |= 8;
	if (v->y < -w) check |= 16;
	if (v->y > w) check |= 32;
	return check;
}

// 归一化，得到屏幕坐标
void transform_homogenize(const transform_t* ts, vector_t* y, const vector_t* x) {
	float rhw = 1.0f / x->w;
	y->x = (x->x * rhw + 1.0f) * ts->w * 0.5f;
	y->y = (1.0f - x->y * rhw) * ts->h * 0.5f;
	y->z = x->z * rhw;
	y->w = 1.0f;
}


//=====================================================================
// 几何计算：顶点、扫描线、边缘、矩形、步长计算
//=====================================================================



void vertex_rhw_init(vertex_t* v) {
	float rhw = 1.0f / v->pos.w;
	v->rhw = rhw;
	v->tc.u *= rhw;
	v->tc.v *= rhw;
	v->color.r *= rhw;
	v->color.g *= rhw;
	v->color.b *= rhw;
}

void vertex_interp(vertex_t* y, const vertex_t* x1, const vertex_t* x2, float t) {
	vector_interp(&y->pos, &x1->pos, &x2->pos, t);
	y->tc.u = interp(x1->tc.u, x2->tc.u, t);
	y->tc.v = interp(x1->tc.v, x2->tc.v, t);
	y->color.r = interp(x1->color.r, x2->color.r, t);
	y->color.g = interp(x1->color.g, x2->color.g, t);
	y->color.b = interp(x1->color.b, x2->color.b, t);
	y->rhw = interp(x1->rhw, x2->rhw, t);
}

void vertex_division(vertex_t* y, const vertex_t* x1, const vertex_t* x2, float w) {
	float inv = 1.0f / w;
	y->pos.x = (x2->pos.x - x1->pos.x) * inv;
	y->pos.y = (x2->pos.y - x1->pos.y) * inv;
	y->pos.z = (x2->pos.z - x1->pos.z) * inv;
	y->pos.w = (x2->pos.w - x1->pos.w) * inv;
	y->tc.u = (x2->tc.u - x1->tc.u) * inv;
	y->tc.v = (x2->tc.v - x1->tc.v) * inv;
	y->color.r = (x2->color.r - x1->color.r) * inv;
	y->color.g = (x2->color.g - x1->color.g) * inv;
	y->color.b = (x2->color.b - x1->color.b) * inv;
	y->rhw = (x2->rhw - x1->rhw) * inv;
}

void vertex_add(vertex_t* y, const vertex_t* x) {
	y->pos.x += x->pos.x;
	y->pos.y += x->pos.y;
	y->pos.z += x->pos.z;
	y->pos.w += x->pos.w;
	y->rhw += x->rhw;
	y->tc.u += x->tc.u;
	y->tc.v += x->tc.v;
	y->color.r += x->color.r;
	y->color.g += x->color.g;
	y->color.b += x->color.b;
}

// 根据三角形生成 0-2 个梯形，并且返回合法梯形的数量
int trapezoid_init_triangle(trapezoid_t* trap, const vertex_t* p1,
	const vertex_t* p2, const vertex_t* p3) {
	const vertex_t* p;
	float k, x;

	if (p1->pos.y > p2->pos.y) p = p1, p1 = p2, p2 = p;
	if (p1->pos.y > p3->pos.y) p = p1, p1 = p3, p3 = p;
	if (p2->pos.y > p3->pos.y) p = p2, p2 = p3, p3 = p;
	if (p1->pos.y == p2->pos.y && p1->pos.y == p3->pos.y) return 0;
	if (p1->pos.x == p2->pos.x && p1->pos.x == p3->pos.x) return 0;

	if (p1->pos.y == p2->pos.y) {	// triangle down
		if (p1->pos.x > p2->pos.x) p = p1, p1 = p2, p2 = p;
		trap[0].top = p1->pos.y;
		trap[0].bottom = p3->pos.y;
		trap[0].left.v1 = *p1;
		trap[0].left.v2 = *p3;
		trap[0].right.v1 = *p2;
		trap[0].right.v2 = *p3;
		return (trap[0].top < trap[0].bottom) ? 1 : 0;
	}

	if (p2->pos.y == p3->pos.y) {	// triangle up
		if (p2->pos.x > p3->pos.x) p = p2, p2 = p3, p3 = p;
		trap[0].top = p1->pos.y;
		trap[0].bottom = p3->pos.y;
		trap[0].left.v1 = *p1;
		trap[0].left.v2 = *p2;
		trap[0].right.v1 = *p1;
		trap[0].right.v2 = *p3;
		return (trap[0].top < trap[0].bottom) ? 1 : 0;
	}

	trap[0].top = p1->pos.y;
	trap[0].bottom = p2->pos.y;
	trap[1].top = p2->pos.y;
	trap[1].bottom = p3->pos.y;

	k = (p3->pos.y - p1->pos.y) / (p2->pos.y - p1->pos.y);
	x = p1->pos.x + (p2->pos.x - p1->pos.x) * k;

	if (x <= p3->pos.x) {		// triangle left
		trap[0].left.v1 = *p1;
		trap[0].left.v2 = *p2;
		trap[0].right.v1 = *p1;
		trap[0].right.v2 = *p3;
		trap[1].left.v1 = *p2;
		trap[1].left.v2 = *p3;
		trap[1].right.v1 = *p1;
		trap[1].right.v2 = *p3;
	}
	else {					// triangle right
		trap[0].left.v1 = *p1;
		trap[0].left.v2 = *p3;
		trap[0].right.v1 = *p1;
		trap[0].right.v2 = *p2;
		trap[1].left.v1 = *p1;
		trap[1].left.v2 = *p3;
		trap[1].right.v1 = *p2;
		trap[1].right.v2 = *p3;
	}

	return 2;
}

// 按照 Y 坐标计算出左右两条边纵坐标等于 Y 的顶点
void trapezoid_edge_interp(trapezoid_t* trap, float y) {
	float s1 = trap->left.v2.pos.y - trap->left.v1.pos.y;
	float s2 = trap->right.v2.pos.y - trap->right.v1.pos.y;
	float t1 = (y - trap->left.v1.pos.y) / s1;
	float t2 = (y - trap->right.v1.pos.y) / s2;
	vertex_interp(&trap->left.v, &trap->left.v1, &trap->left.v2, t1);
	vertex_interp(&trap->right.v, &trap->right.v1, &trap->right.v2, t2);
}

// 根据左右两边的端点，初始化计算出扫描线的起点和步长
void trapezoid_init_scan_line(const trapezoid_t* trap, scanline_t* scanline, int y) {
	float width = trap->right.v.pos.x - trap->left.v.pos.x;
	scanline->x = (int)(trap->left.v.pos.x + 0.5f);
	scanline->w = (int)(trap->right.v.pos.x + 0.5f) - scanline->x;
	scanline->y = y;
	scanline->v = trap->left.v;
	if (trap->left.v.pos.x >= trap->right.v.pos.x) scanline->w = 0;
	vertex_division(&scanline->step, &trap->left.v, &trap->right.v, width);
}


//=====================================================================
// 渲染设备
//=====================================================================
typedef struct {
	transform_t transform;      // 坐标变换器
	int width;                  // 窗口宽度
	int height;                 // 窗口高度
	IUINT32** framebuffer;      // 像素缓存：framebuffer[y] 代表第 y行
	float** zbuffer;            // 深度缓存：zbuffer[y] 为第 y行指针
	IUINT32** texture;          // 纹理：同样是每行索引
	int tex_width;              // 纹理宽度
	int tex_height;             // 纹理高度
	float max_u;                // 纹理最大宽度：tex_width - 1
	float max_v;                // 纹理最大高度：tex_height - 1
	int render_state;           // 渲染状态
	IUINT32 background;         // 背景颜色
	IUINT32 foreground;         // 线框颜色
}	device_t;

#define RENDER_STATE_WIREFRAME      1		// 渲染线框
#define RENDER_STATE_TEXTURE        2		// 渲染纹理
#define RENDER_STATE_TEXTURE_BACK_BLANKING	8 //渲染纹理（背面消隐）
#define RENDER_STATE_COLOR          4		// 渲染颜色


// 设备初始化，fb为外部帧缓存，非 NULL 将引用外部帧缓存（每行 4字节对齐）
void device_init(device_t * device, int width, int height, void* fb) {
	int need = sizeof(void*) * (height * 2 + 1024) + width * height * 8;
	char* ptr = (char*)malloc(need + 64);
	char* framebuf, * zbuf;
	int j;
	assert(ptr);
	device->framebuffer = (IUINT32 * *)ptr;
	device->zbuffer = (float**)(ptr + sizeof(void*) * height);
	ptr += sizeof(void*) * height * 2;
	device->texture = (IUINT32 * *)ptr;
	ptr += sizeof(void*) * 1024;
	framebuf = (char*)ptr;
	zbuf = (char*)ptr + width * height * 4;
	ptr += width * height * 8;
	if (fb != NULL) framebuf = (char*)fb;
	for (j = 0; j < height; j++) {
		device->framebuffer[j] = (IUINT32*)(framebuf + width * 4 * j);
		device->zbuffer[j] = (float*)(zbuf + width * 4 * j);
	}
	device->texture[0] = (IUINT32*)ptr;
	device->texture[1] = (IUINT32*)(ptr + 16);
	memset(device->texture[0], 0, 64);
	device->tex_width = 2;
	device->tex_height = 2;
	device->max_u = 1.0f;
	device->max_v = 1.0f;
	device->width = width;
	device->height = height;
	device->background = 0xc0c0c0;
	device->foreground = 0;
	transform_init(&device->transform, width, height);
	device->render_state = RENDER_STATE_WIREFRAME;
}

// 删除设备
void device_destroy(device_t* device) {
	if (device->framebuffer)
		free(device->framebuffer);
	device->framebuffer = NULL;
	device->zbuffer = NULL;
	device->texture = NULL;
}


// 设置当前纹理


// 设置当前纹理
void device_set_texture(device_t* device, void* bits, long pitch, int w, int h) {
	char* ptr = (char*)bits;
	int j;
	//assert(w <= 1024 && h <= 1024);
	//cout <<"pitch " <<pitch << endl;
	for (j = 0; j < h; ptr += pitch, j++) 	// 重新计算每行纹理的指针
	{
		device->texture[j] = (IUINT32*)ptr;
	}

	device->tex_width = w;
	device->tex_height = h;
	device->max_u = (float)(w - 1);
	device->max_v = (float)(h - 1);
	//cout << "set_texture w " << w << " h " << h << endl;
}

// 清空 framebuffer 和 zbuffer
void device_clear(device_t* device, int mode) {
	int y, x, height = device->height;
	for (y = 0; y < device->height; y++) {
		IUINT32* dst = device->framebuffer[y];
		IUINT32 cc = (height - 1 - y) * 230 / (height - 1);
		cc = (cc << 16) | (cc << 8) | cc;
		if (mode == 0) cc = device->background;
		for (x = device->width; x > 0; dst++, x--) dst[0] = cc;
	}
	for (y = 0; y < device->height; y++) {
		float* dst = device->zbuffer[y];
		for (x = device->width; x > 0; dst++, x--) dst[0] = 0.0f;
	}
}

// 画点
void device_pixel(device_t* device, int x, int y, IUINT32 color) {
	if (((IUINT32)x) < (IUINT32)device->width && ((IUINT32)y) < (IUINT32)device->height) {
		device->framebuffer[y][x] = color;
	}
}

// 绘制线段
void device_draw_line(device_t* device, int x1, int y1, int x2, int y2, IUINT32 c) {
	int x, y, rem = 0;
	if (x1 == x2 && y1 == y2) {
		device_pixel(device, x1, y1, c);
	}
	else if (x1 == x2) {
		int inc = (y1 <= y2) ? 1 : -1;
		for (y = y1; y != y2; y += inc) device_pixel(device, x1, y, c);
		device_pixel(device, x2, y2, c);
	}
	else if (y1 == y2) {
		int inc = (x1 <= x2) ? 1 : -1;
		for (x = x1; x != x2; x += inc) device_pixel(device, x, y1, c);
		device_pixel(device, x2, y2, c);
	}
	else {
		int dx = (x1 < x2) ? x2 - x1 : x1 - x2;
		int dy = (y1 < y2) ? y2 - y1 : y1 - y2;
		if (dx >= dy) {
			if (x2 < x1) x = x1, y = y1, x1 = x2, y1 = y2, x2 = x, y2 = y;
			for (x = x1, y = y1; x <= x2; x++) {
				device_pixel(device, x, y, c);
				rem += dy;
				if (rem >= dx) {
					rem -= dx;
					y += (y2 >= y1) ? 1 : -1;
					device_pixel(device, x, y, c);
				}
			}
			device_pixel(device, x2, y2, c);
		}
		else {
			if (y2 < y1) x = x1, y = y1, x1 = x2, y1 = y2, x2 = x, y2 = y;
			for (x = x1, y = y1; y <= y2; y++) {
				device_pixel(device, x, y, c);
				rem += dx;
				if (rem >= dy) {
					rem -= dy;
					x += (x2 >= x1) ? 1 : -1;
					device_pixel(device, x, y, c);
				}
			}
			device_pixel(device, x2, y2, c);
		}
	}
}

// 根据坐标读取纹理
IUINT32 device_texture_read(const device_t* device, float u, float v) {
	int x, y;
	//cout << "u " << u << " v " << v << endl;
	u = u * device->max_u;
	v = v * device->max_v;
	x = (int)(u + 0.5f);
	y = (int)(v + 0.5f);
	x = CMID(x, 0, device->tex_width - 1);
	y = CMID(y, 0, device->tex_height - 1);

	//cout << "x " << x << " y " << y << endl;
	//cout << device->texture[y][x] << endl;
	UINT32 cc = device->texture[y][x];

	int R = cc >> 16;
	int G = (cc >> 8);
	G %= 256;
	int B = cc % 256;
	//cout << "R " << R << " G " << G << " B " << B << endl;

	return device->texture[y][x];
}


//=====================================================================
// 渲染实现
//=====================================================================
vector_t normal;

// 绘制扫描线
void device_draw_scanline(device_t* device, scanline_t* scanline) {
	IUINT32* framebuffer = device->framebuffer[scanline->y];
	float* zbuffer = device->zbuffer[scanline->y];
	int x = scanline->x;
	int w = scanline->w;
	int width = device->width;
	int render_state = device->render_state;
	vector_t light_s;

	//这里的scanline->v.pos已经是归一化的了 需要未归一化的那个
	vector_sub(&light_s, &light_soure_camera, &scanline->v.pos);

	//show_vector(light_soure_camera);

	//show_vector(center_p);



	//show_vector(light_s);

	//计算镜面反射
	vector_t e;

	

	vector_scale(&e, &scanline->v.pos, -1);

	//cout << "e ";
	//show_vector(e);
	vector_normalize(&e);

	//h=(e+l)/|||e+l||
	vector_t temp_up, temp_down;
	float temp_down_value;
	vector_t h;
	vector_add(&temp_up, &e, &light_s);
	temp_down_value=vector_length(&temp_up);
	vector_scale(&h, &temp_up, 1/temp_down_value);
	float phong;
	vector_normalize(&h);
	vector_normalize(&normal);
	phong=vector_dotproduct(&h, &normal);
	phong = max(0, phong);
	//cout << "phong " << phong << endl;



	//cos(angle)=(vector1*vector2)/(|vector1|*|vector2|)
	float cosAngle=0;
	cosAngle += max(0, vector_dotproduct(&normal, &light_s) / (vector_length(&normal) * vector_length(&light_s)));

	cosAngle += 0.3;	//自发光
	//cosAngle += phong*0.1;
	
	cosAngle = min(cosAngle, 1);


	//cout <<"width "<< width << endl;
	for (; w > 0; x++, w--) {
		if (x >= 0 && x < width) {
			float rhw = scanline->v.rhw;
			if (rhw >= zbuffer[x]) {
				float w = 1.0f / rhw;
				zbuffer[x] = rhw;
				if (render_state & RENDER_STATE_COLOR) {
					float r = scanline->v.color.r * w;
					float g = scanline->v.color.g * w;
					float b = scanline->v.color.b * w;
					int R = (int)(r * 255.0f);
					int G = (int)(g * 255.0f);
					int B = (int)(b * 255.0f);
					R = CMID(R, 0, 255);
					G = CMID(G, 0, 255);
					B = CMID(B, 0, 255);
					framebuffer[x] = (R << 16) | (G << 8) | (B);
				}
				if (render_state & RENDER_STATE_TEXTURE) {


					float u = scanline->v.tc.u * w;
					float v = scanline->v.tc.v * w;

					IUINT32 cc = device_texture_read(device, u, v);
					//cout << "cc" << cc << endl;
					int B = cc >> 16;
					int G = (cc >> 8);
					G %= 256;
					int R = cc % 256;

					R *= cosAngle;
					G *= cosAngle;
					B *= cosAngle;

					//cout << "y " << scanline->y << " x " << scanline->x << " u " << scanline->v.tc.u << " v " << scanline->v.tc.v << endl;
					framebuffer[x] = (R << 16) | (G << 8) | (B);

				}
			}
		}
		vertex_add(&scanline->v, &scanline->step);
		if (x >= width) break;
	}
}

// 主渲染函数
void device_render_trap(device_t* device, trapezoid_t* trap) {
	scanline_t scanline;
	int j, top, bottom;
	top = (int)(trap->top + 0.5f);
	bottom = (int)(trap->bottom + 0.5f);
	for (j = top; j < bottom; j++) {
		if (j >= 0 && j < device->height) {
			trapezoid_edge_interp(trap, (float)j + 0.5f);
			trapezoid_init_scan_line(trap, &scanline, j);
			device_draw_scanline(device, &scanline);
		}
		if (j >= device->height) break;
	}
}



// 根据 render_state 绘制原始三角形
void device_draw_primitive(device_t* device, const vertex_t* v1,
	const vertex_t* v2, const vertex_t* v3) {
	point_t p1, p2, p3, c1, c2, c3;
	int render_state = device->render_state;


	// 按照 Transform 变化
	transform_apply(&device->transform, &c1, &v1->pos);
	transform_apply(&device->transform, &c2, &v2->pos);
	transform_apply(&device->transform, &c3, &v3->pos);

	// 裁剪，注意此处可以完善为具体判断几个点在 cvv内以及同cvv相交平面的坐标比例
	// 进行进一步精细裁剪，将一个分解为几个完全处在 cvv内的三角形
	if (transform_check_cvv(&c1) != 0) return;
	if (transform_check_cvv(&c2) != 0) return;
	if (transform_check_cvv(&c3) != 0) return;



	// 归一化
	transform_homogenize(&device->transform, &p1, &c1);
	transform_homogenize(&device->transform, &p2, &c2);
	transform_homogenize(&device->transform, &p3, &c3);



	//背面剔除
	//法向量坐标值小于0的三角形剔除(默认开启)

	double z = (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
	if (z < 0)
		return;

	// 纹理或者色彩绘制
	if (render_state & (RENDER_STATE_TEXTURE | RENDER_STATE_COLOR | RENDER_STATE_TEXTURE_BACK_BLANKING)) {
		vertex_t t1 = *v1, t2 = *v2, t3 = *v3;
		trapezoid_t traps[2];
		int n;

		t1.pos = p1;
		t2.pos = p2;
		t3.pos = p3;
		t1.pos.w = c1.w;
		t2.pos.w = c2.w;
		t3.pos.w = c3.w;

		vertex_rhw_init(&t1);	// 初始化 w
		vertex_rhw_init(&t2);	// 初始化 w
		vertex_rhw_init(&t3);	// 初始化 w


		// 顶点光照

		vector_t l1;
		vector_t l2;
		point_t center_p;


		vector_sub(&l1, &(c1), &(c2));
		vector_sub(&l2, &(c1), &(c3));
		vector_crossproduct(&normal, &l1, &l2);


		//show_vector(normal);

		vector_add(&center_p, &(c1), &(c2));
		vector_add(&center_p, &center_p, &(c3));
		vector_scale(&center_p, &center_p, 0.33333);


		/*   第一次画三角形
		if (first_tri == true)
		{
			first_tri = false;
			cout << "light_s ";
			show_vector(light_s);
			cout << "center tri ";
			show_vector(center_p);
			cout << "light_soure_camera ";
			show_vector(light_soure_camera);
			cout << "normal ";
			show_vector(normal);
			cout << "light " << cosAngle << endl;

		}
		*/


		// 拆分三角形为0-2个梯形，并且返回可用梯形数量
		n = trapezoid_init_triangle(traps, &t1, &t2, &t3);

		if (n >= 1) device_render_trap(device, &traps[0]);
		if (n >= 2) device_render_trap(device, &traps[1]);
	}

	if (render_state & RENDER_STATE_WIREFRAME) {		// 线框绘制
		device_draw_line(device, (int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, device->foreground);
		device_draw_line(device, (int)p1.x, (int)p1.y, (int)p3.x, (int)p3.y, device->foreground);
		device_draw_line(device, (int)p3.x, (int)p3.y, (int)p2.x, (int)p2.y, device->foreground);
	}
}


//=====================================================================
// Win32 窗口及图形绘制：为 device 提供一个 DibSection 的 FB
//=====================================================================
int screen_w, screen_h, screen_exit = 0;
int screen_mx = 0, screen_my = 0, screen_mb = 0;
int screen_keys[512];	// 当前键盘按下状态
static HWND screen_handle = NULL;		// 主窗口 HWND
static HDC screen_dc = NULL;			// 配套的 HDC
static HBITMAP screen_hb = NULL;		// DIB
static HBITMAP screen_ob = NULL;		// 老的 BITMAP
unsigned char* screen_fb = NULL;		// frame buffer
long screen_pitch = 0;

int screen_init(int w, int h, const TCHAR* title);	// 屏幕初始化
int screen_close(void);								// 关闭屏幕
void screen_dispatch(void);							// 处理消息
void screen_update(void);							// 显示 FrameBuffer

// win32 event handler
static LRESULT screen_events(HWND, UINT, WPARAM, LPARAM);

#ifdef _MSC_VER
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#endif

// 初始化窗口并设置标题
int screen_init(int w, int h, const TCHAR* title) {
	WNDCLASS wc = { CS_BYTEALIGNCLIENT, (WNDPROC)screen_events, 0, 0, 0,
		NULL, NULL, NULL, NULL, _T("SCREEN3.1415926") };
	BITMAPINFO bi = { { sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB,
		w * h * 4, 0, 0, 0, 0 } };
	RECT rect = { 0, 0, w, h };
	int wx, wy, sx, sy;
	LPVOID ptr;
	HDC hDC;

	screen_close();

	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	if (!RegisterClass(&wc)) return -1;

	screen_handle = CreateWindow(_T("SCREEN3.1415926"), title,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);
	if (screen_handle == NULL) return -2;

	screen_exit = 0;
	hDC = GetDC(screen_handle);
	screen_dc = CreateCompatibleDC(hDC);
	ReleaseDC(screen_handle, hDC);

	screen_hb = CreateDIBSection(screen_dc, &bi, DIB_RGB_COLORS, &ptr, 0, 0);
	if (screen_hb == NULL) return -3;

	screen_ob = (HBITMAP)SelectObject(screen_dc, screen_hb);
	screen_fb = (unsigned char*)ptr;
	screen_w = w;
	screen_h = h;
	screen_pitch = w * 4;

	AdjustWindowRect(&rect, GetWindowLong(screen_handle, GWL_STYLE), 0);
	wx = rect.right - rect.left;
	wy = rect.bottom - rect.top;
	sx = (GetSystemMetrics(SM_CXSCREEN) - wx) / 2;
	sy = (GetSystemMetrics(SM_CYSCREEN) - wy) / 2;
	if (sy < 0) sy = 0;
	SetWindowPos(screen_handle, NULL, sx, sy, wx, wy, (SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW));
	SetForegroundWindow(screen_handle);

	ShowWindow(screen_handle, SW_NORMAL);
	screen_dispatch();

	memset(screen_keys, 0, sizeof(int) * 512);
	memset(screen_fb, 0, w * h * 4);

	return 0;
}

int screen_close(void) {
	if (screen_dc) {
		if (screen_ob) {
			SelectObject(screen_dc, screen_ob);
			screen_ob = NULL;
		}
		DeleteDC(screen_dc);
		screen_dc = NULL;
	}
	if (screen_hb) {
		DeleteObject(screen_hb);
		screen_hb = NULL;
	}
	if (screen_handle) {
		CloseWindow(screen_handle);
		screen_handle = NULL;
	}
	return 0;
}

static LRESULT screen_events(HWND hWnd, UINT msg,
	WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CLOSE: screen_exit = 1; break;
	case WM_KEYDOWN: screen_keys[wParam & 511] = 1; break;
	case WM_KEYUP: screen_keys[wParam & 511] = 0; break;
	default: return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

void screen_dispatch(void) {
	MSG msg;
	while (1) {
		if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) break;
		if (!GetMessage(&msg, NULL, 0, 0)) break;
		DispatchMessage(&msg);
	}
}

void screen_update(void) {
	HDC hDC = GetDC(screen_handle);
	BitBlt(hDC, 0, 0, screen_w, screen_h, screen_dc, 0, 0, SRCCOPY);
	ReleaseDC(screen_handle, hDC);
	screen_dispatch();
}


//=====================================================================
// 主程序
//=====================================================================

void DrawTexture(device_t* device)
{
	for (float y = 0; y < device->tex_height; y++)
	{

		for (float x = 0; x < device->tex_width; x++)
		{
			float u = x / device->tex_width;
			float v = y / device->tex_height;
			IUINT32 color = device_texture_read(device, u, v);

			device->framebuffer[(int)y][(int)x] = color;
		}
	}



}



void draw_object(device_t* device, Object* object)
{
	device_set_texture(device, object->texture, object->tex_width * 4, object->tex_width, object->tex_height);

	matrix_t mlight;
	matrix_set_translate(&mlight, 0, 0, 0);
	device->transform.world = mlight;
	transform_update(&device->transform);

	transform_apply(&device->transform, &light_soure_camera, &light_soure);
	//show_vector(light_soure_camera);

	matrix_t m1, m2;

	matrix_set_translate(&m1, object->pos.x, object->pos.y, object->pos.z);



	matrix_set_rotate(&m2, 0, 1, 0, model_angle);
	matrix_mul(&m2, &m2, &m1);
	device->transform.world = m2;
	transform_update(&device->transform);
	first_tri = true;



	for (int i = 0; i < object->TriangleIndexSum; i++)
	{
		//画三角形
		vertex_t p1, p2, p3;
		p1.pos = object->point[object->triangleIndex[i].pointA - 1];
		p1.pos.w = 1;
		p1.tc = object->uv[object->triangleIndex[i].uvA - 1];
		p1.vn = object->vn[object->triangleIndex[i].vnA - 1];
		p2.pos = object->point[object->triangleIndex[i].pointB - 1];
		p2.pos.w = 1;
		p2.tc = object->uv[object->triangleIndex[i].uvB - 1];
		p2.vn = object->vn[object->triangleIndex[i].vnB - 1];
		p3.pos = object->point[object->triangleIndex[i].pointC - 1];
		p3.pos.w = 1;
		p3.tc = object->uv[object->triangleIndex[i].uvC - 1];
		p3.vn = object->vn[object->triangleIndex[i].vnC - 1];
		//cout << "233" << endl;
		//cout << "p1 pos " << p1.pos.x << " " << p1.pos.y << " " << p1.pos.z << endl;
		//cout << "u " << p1.tc.u << " v " << p1.tc.v << endl;
		//cout << "p2 pos " << p2.pos.x << " " << p2.pos.y << " " << p2.pos.z << endl;
		//cout << "u " << p2.tc.u << " v " << p2.tc.v << endl;
		//cout << "p3 pos " << p3.pos.x << " " << p3.pos.y << " " << p3.pos.z << endl;
		//cout << "u " << p3.tc.u << " v " << p3.tc.v << endl;
		device_draw_primitive(device, &p1, &p2, &p3);

	}

}


void update_camera_matrix(device_t* device)
{
	matrix_t matView;
	pCamera->CalculateViewMatrix(&matView);

	device->transform.view = matView;
	transform_update(&device->transform);

}

void init_texture(device_t* device) {
	static IUINT32 texture[256][256];
	int i, j;
	for (j = 0; j < 256; j++) {
		for (i = 0; i < 256; i++) {
			int x = i / 32, y = j / 32;
			texture[j][i] = ((x + y) & 1) ? 0xffffff : 0x3fbcef;
		}
	}
	device_set_texture(device, texture, 256 * 4, 256, 256);
}

bool init_texture(Object* object, const char* filename)
{
	/*

	RGBQUAD* pColorTable;//颜色表指针


	FILE* fp = fopen(filename, "rb");//二进制读方式打开指定的图像文件
	if (fp == 0)
		return 0;


	//跳过位图文件头结构BITMAPFILEHEADER
	fseek(fp, sizeof(BITMAPFILEHEADER), 0);
	//
	BITMAPFILEHEADER filehead;
	fread(&filehead, 1, sizeof(BITMAPFILEHEADER), fp);
	showBmpHead(filehead);//显示文件头
	//

//定义位图信息头结构变量，读取位图信息头进内存，存放在变量head中
	BITMAPINFOHEADER infohead;
	fread(&infohead, sizeof(BITMAPINFOHEADER), 1, fp); //获取图像宽、高、每像素所占位数等信息
	device->tex_width  = infohead.biWidth;
	device->tex_height = infohead.biHeight;
	int biBitCount = infohead.biBitCount;//定义变量，计算图像每行像素所占的字节数（必须是4的倍数）
	//showBmpInforHead(infohead);//显示信息头


	int lineByte = (device->tex_width * biBitCount / 8 + 3) / 4 * 4;//灰度图像有颜色表，且颜色表表项为256
	if (biBitCount == 8)
	{
		//申请颜色表所需要的空间，读颜色表进内存
		pColorTable = new RGBQUAD[256];
		fread(pColorTable, sizeof(RGBQUAD), 256, fp);
	}

	//申请位图数据所需要的空间，读位图数据进内存
	unsigned char* pBmpBuf = new unsigned char[lineByte * device->tex_height];
	fread(pBmpBuf, 1, lineByte * device->tex_height, fp);
	fclose(fp);//关闭文件

	static IUINT32 *texture;
	texture = new IUINT32  [device->tex_height*device->tex_width];

	int i, j;
	for (j = 0; j < device->tex_height; j++) {
		for (i = 0; i < device->tex_width; i++) {
			//int x = i / 32, y = j / 32;
			int R, G, B;
			R=pBmpBuf[j * 3 * device->tex_width + i * 3 + 2];
			G=pBmpBuf[j * 3 * device->tex_width + i * 3 + 1];
			B=pBmpBuf[j * 3 * device->tex_width + i * 3];
			texture[j*device->tex_width+i] = (R<<16)|(G<<8)|B;
		}
	}
	device_set_texture(device, texture, device->tex_width * 4, device->tex_width, device->tex_height);
	*/
	CImage cImage;

	cImage.Load(filename);
	byte* pImg = (byte*)cImage.GetBits();
	int imgWidth = cImage.GetWidth();
	int imgHeight = cImage.GetHeight();
	object->tex_width = imgWidth;
	object->tex_height = imgHeight;
	int step = cImage.GetPitch();
	object->texture = new IUINT32[object->tex_height * object->tex_width];



	for (int y = 0; y < imgHeight; y++)
	{
		for (int x = 0; x < imgWidth; x++)
		{
			COLORREF color;
			color = cImage.GetPixel(x, imgHeight - 1 - y);


			object->texture[y * object->tex_width + x] = color;
			//cout << "R " <<(int) GetRValue(color) << " G " <<(int) GetGValue(color) << " B " <<(int) GetBValue(color) << endl;

			//pBmpBuf[(y * imgWidth + x) * 3] =(char) GetRValue(color);
			//pBmpBuf[(y * imgWidth + x) * 3 + 1] =(char) GetGValue(color);
			//pBmpBuf[(y * imgWidth + x) * 3+2] =(char)GetBValue(color);
			//pBmpBuf[int(y) * imgWidth * 3 + (int(x) + 1) * 3 - 1] = R;
			//pBmpBuf[int(y) * imgWidth * 3 + (int(x) + 1) * 3 - 2] = G;
			//pBmpBuf[int(y) * imgWidth * 3 + (int(x) + 1) * 3 - 3] = B;
		}
	}

	//device_set_object(object,object-> texture, object->tex_width * 4, object->tex_width, object->tex_height);

	//cout << "666" << endl;
	return 1;//读取文件成功
}

int main(void)
{
	device_t device;
	int states[] = { RENDER_STATE_TEXTURE, RENDER_STATE_COLOR,RENDER_STATE_TEXTURE_BACK_BLANKING, RENDER_STATE_WIREFRAME };
	int indicator = 0;
	int kbhit = 0;
	float alpha = 1;
	float pos = 3.5;


	pCamera = new Camera();
	vector_t CamerPos = vector_t(0.0f, 0.0f, 0.0f);
	vector_t TargetPos = vector_t(0.0f, 0.0f, 5.0f);
	pCamera->SetCameraPosition(&CamerPos);
	pCamera->SetTargetPosition(&TargetPos);
	pCamera->SetViewMatrix();
	pCamera->SetProjMatrix();



	char title[] = "Mini3d  Render "
		"Left/Right: rotation, Up/Down: forward/backward, Space: switch state";

	if (screen_init(800, 600, title))
		return -1;

	device_init(&device, 800, 600, screen_fb);
	update_camera_matrix(&device);


	device.render_state = RENDER_STATE_TEXTURE;
	//Object earth= LoadObject("earth.obj");
	//init_texture(&earth, "Diffuse_2K.png");
	//earth.pos = vector_t(5, 10, 0);

	//Object fighter = LoadObject("fighter.obj");
	//init_texture(&fighter, "yellow.bmp");
	//fighter.pos = vector_t(0, -2, 0);

	Object car = LoadObject("hello.obj");
	init_texture(&car, "yellow.bmp");
	car.pos = vector_t(0, 0, 100);

	int frame = 0;
	time_t start, stop;
	start = time(NULL);

	while (screen_exit == 0 && screen_keys[VK_ESCAPE] == 0) {

		frame++;
		stop = time(NULL);
		if (stop - start >= 1)
		{
			start = stop;
			char title[200];
			vector_t camPos;
			pCamera->GetCameraPosition(&camPos);
			sprintf(title, "FPS %d POS (%.1f %.1f %.1f) ", frame, camPos.x, camPos.y, camPos.z);
			SetWindowText(screen_handle, title);
			frame = 0;
		}

		light_soure.x = cos(light_angle_x) * sin(light_angle_y) * 100;
		light_soure.y = sin(light_angle_x) * cos(light_angle_y) * 100;
		light_soure.z = 100 * cos(light_angle_x);

		//Camera 设置
		screen_dispatch();
		device_clear(&device, 1);
		update_camera_matrix(&device);

		if (screen_keys[VK_UP])
		{
			pCamera->RotationRightVec(-0.01f);
		}
		if (screen_keys[VK_DOWN])
		{
			pCamera->RotationRightVec(0.01f);
		}
		if (screen_keys[VK_LEFT])
		{
			pCamera->RotationUpVec(-0.02f);
		}
		if (screen_keys[VK_RIGHT])
		{
			pCamera->RotationUpVec(0.02f);
		}
		if (screen_keys['W'])pCamera->MoveAlongLookVec(0.1);
		if (screen_keys['S'])pCamera->MoveAlongLookVec(-0.1);
		if (screen_keys['A'])pCamera->MoveAlongRightVec(-0.1);
		if (screen_keys['D'])pCamera->MoveAlongRightVec(0.1);
		if (screen_keys['Q'])pCamera->MoveAlongUpVec(0.1);
		if (screen_keys['E'])
		{
			vector_t vecPos;

			pCamera->GetCameraPosition(&vecPos);

			pCamera->MoveAlongUpVec(-0.1);

		}
		if (screen_keys['I'])
		{
			light_angle_x += 0.01;
			if (light_angle_x > 2 * 3.1415926)
			{
				light_angle_x = 0;
			}
		}
		if (screen_keys['O'])
		{
			light_angle_x -= 0.01;
			if (light_angle_x < 0)
			{
				light_angle_x = 2 * 3.1415926;
			}
		}
		if (screen_keys['K'])
		{
			pCamera->RotationLookVec(0.01f);
		}
		if (screen_keys['L'])
		{
			pCamera->RotationLookVec(-0.01f);
		}
		if (screen_keys[VK_SPACE]) {
			if (kbhit == 0) {
				kbhit = 1;
				if (++indicator >= 4) indicator = 0;
				device.render_state = states[indicator];
			}
		}
		else {
			kbhit = 0;
		}
		//light_angle_x += 0.002;
		if (light_angle_x > 2 * 3.1415926)
		{
			light_angle_x = 0;
		}
		//light_angle_y -= 0.002;
		if (light_angle_y < 0)
		{
			light_angle_y = 2 * 3.1415926;
		}
		model_angle += 0.005;
		if (model_angle > 2 * 3.1415926)
		{
			model_angle = 0;
		}


		//draw_box(&device, alpha);
		//DrawTexture(&device);
		//draw_object(&device, &earth);
		//draw_object(&device, &fighter);
		draw_object(&device, &car);
		screen_update();
		Sleep(1);
	}
	return 0;
}
