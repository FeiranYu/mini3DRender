#include"main.h"
#include<vector>

int GetInt(const char* str, int size, int &now)
{
	int num = 0;
	for (int i = now; i < size; i++)
	{
		if (str[i] == '/')
		{
			now = i + 1;
			break;
		}
		else
		{
			num *= 10;
			num += str[i] - '0';
		}
	}
	return num;
}


Object LoadObject(const char *filename)
{
	ifstream infile;
	infile.open(filename, ios::out || ios::in);

	Object object;

	string str;

	vector<vector_t> pointVec;
	vector<texcoord_t> uvVec;
	vector<vector_t> vnVec;
	vector<TriangleIndex_t> triangleIndexVec;

	while (1)
	{
		infile >> str;
		
		if (infile.eof())
		{
			break;
		}
		else if (strcmp(str.c_str(), "v")==0)	//vertex
		{
			vector_t point;
			infile >> point.x >> point.y >> point.z;
			pointVec.push_back(point);
		}
		else if (strcmp(str.c_str(), "vt") == 0)	//vertex texture
		{
			texcoord_t uv;
			infile >> uv.u >> uv.v;
			uvVec.push_back(uv);
		}
		else if (strcmp(str.c_str(), "vn") == 0)	//vertex normal
		{
			vector_t vertexnormal;
			infile >> vertexnormal.x >> vertexnormal.y >> vertexnormal.z;
			vnVec.push_back(vertexnormal);
		}
		else if (strcmp(str.c_str(), "f") == 0)
		{
			TriangleIndex_t triangleIndex;
			string strTri1;
			string strTri2;
			string strTri3;
			infile >> strTri1 >> strTri2 >> strTri3;
			int nowScanInt = 0;

			triangleIndex.pointA = GetInt(strTri1.c_str(), strTri1.length(), nowScanInt);
			triangleIndex.uvA = GetInt(strTri1.c_str(), strTri1.length(), nowScanInt);
			triangleIndex.vnA = GetInt(strTri1.c_str(), strTri1.length(), nowScanInt);

			nowScanInt = 0;
			triangleIndex.pointB = GetInt(strTri2.c_str(), strTri2.length(), nowScanInt);
			triangleIndex.uvB = GetInt(strTri2.c_str(), strTri2.length(), nowScanInt);
			triangleIndex.vnB = GetInt(strTri2.c_str(), strTri2.length(), nowScanInt);

			nowScanInt = 0;
			triangleIndex.pointC = GetInt(strTri3.c_str(), strTri3.length(), nowScanInt);
			triangleIndex.uvC = GetInt(strTri3.c_str(), strTri3.length(), nowScanInt);
			triangleIndex.vnC = GetInt(strTri3.c_str(), strTri3.length(), nowScanInt);
			triangleIndexVec.push_back(triangleIndex);
		}
		else {
			cout << str << endl;
		}
		
	}
	cout<<"point sum " << pointVec.size() << endl;
	cout << "vertex texture " << uvVec.size() << endl;
	cout << "vertex normal " << vnVec.size() << endl;
	cout << "triangle sum " << triangleIndexVec.size() << endl;

	object.pointSum = pointVec.size();
	object.uvSum = uvVec.size();
	object.vnSum = vnVec.size();
	object.TriangleIndexSum = triangleIndexVec.size();
	
	object.point = new point_t[object.pointSum];
	object.uv = new texcoord_t[object.uvSum];
	object.vn = new point_t[object.vnSum];
	object.triangleIndex = new TriangleIndex_t[object.TriangleIndexSum];

	for (int i = 0; i < object.pointSum; i++)
	{
		object.point[i] = pointVec[i];
		//cout << "point[" << i << "] x " << pointVec[i].x << " y " << pointVec[i].y << " z " << pointVec[i].z << endl;
	}
	for (int i = 0; i < object.uvSum; i++)
	{
		object.uv[i] = uvVec[i];
	}
	for (int i = 0; i < object.vnSum; i++)
	{
		object.vn[i] = vnVec[i];
	}
	for (int i = 0; i < object.TriangleIndexSum; i++)
	{
		object.triangleIndex[i] = triangleIndexVec[i];
		//cout << "triangleIndex A " << triangleIndexVec[i].pointA << " B " << triangleIndexVec[i].pointB << " C " << triangleIndexVec[i].pointC << endl;
	}
	
	infile.close();
	return object;
}