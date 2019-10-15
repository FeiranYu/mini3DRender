#pragma once
#include"main.h"

class Camera
{
private:
	vector_t m_vRightVector;
	vector_t m_vUpVector;
	vector_t m_vLookVector;
	vector_t m_vCameraPosition;
	vector_t m_vTargetPosition;
	matrix_t m_matView;			//ȡ���任����
	matrix_t m_matPorj;			//ͶӰ�任����
public:
	void CalculateViewMatrix(matrix_t* pMatrix);	//����ȡ���任����

	void GetPorjMatrix(matrix_t* pMatrix) { *pMatrix = m_matPorj; }
	void GetCameraPosition(vector_t *pVector) { *pVector = m_vCameraPosition; }
	void GetLookVector(vector_t* pVector) { *pVector = m_vLookVector; }

	void SetTargetPosition(vector_t* pLookat = NULL);
	void SetCameraPosition(vector_t* pVector = NULL);
	void SetViewMatrix(matrix_t* pMatrix = NULL);
	void SetProjMatrix(matrix_t* pMatrix = NULL);

public:
	//�ظ�����ƽ�Ƶ���������
	void MoveAlongRightVec(float fUnits);
	void MoveAlongUpVec(float fUnits);
	void MoveAlongLookVec(float fUnits);

	//�Ƹ�������ת����������
	void RotationRightVec(float fAngle);
	void RotationUpVec(float fAngle);
	void RotationLookVec(float fAngle);

public:
	Camera();
	virtual ~Camera(void);
};