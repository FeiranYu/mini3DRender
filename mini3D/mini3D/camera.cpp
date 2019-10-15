#include"camera.h"

Camera::Camera()
{
	m_vRightVector.x = 1.0f; m_vRightVector.y = 0.0f; m_vRightVector.z = 0.0f; m_vRightVector.w = 1.0f;
	m_vUpVector.x = 0.0f; m_vUpVector.y = 1.0f; m_vUpVector.z = 0.0f; m_vUpVector.w = 1.0f;
	m_vLookVector.x = 0.0f; m_vLookVector.y = 0.0f; m_vLookVector.z = 1.0f; m_vLookVector.w = 1.0f;
	m_vCameraPosition.x = 0.0f; m_vCameraPosition.y = 0.0f; m_vCameraPosition.z = -250.0f; m_vCameraPosition.w = 1.0f;
	m_vTargetPosition.x = 0.0f; m_vTargetPosition.y = 0.0f; m_vTargetPosition.z = 0.0f; m_vTargetPosition.w = 0.0f;

}

void Camera::CalculateViewMatrix(matrix_t* pMatrix)
{
	vector_normalize(&m_vLookVector);
	vector_crossproduct(&m_vUpVector, &m_vLookVector, &m_vRightVector);
	vector_normalize(&m_vUpVector);
	vector_crossproduct(&m_vRightVector, &m_vUpVector, &m_vLookVector);
	vector_normalize(&m_vRightVector);

	pMatrix->m[0][0] = m_vRightVector.x;		//Rx
	pMatrix->m[0][1] = m_vUpVector.x;			//Ux
	pMatrix->m[0][2] = m_vLookVector.x;			//Lx
	pMatrix->m[0][3] = 0.0f;

	pMatrix->m[1][0] = m_vRightVector.y;
	pMatrix->m[1][1] = m_vUpVector.y;
	pMatrix->m[1][2] = m_vLookVector.y;
	pMatrix->m[1][3] = 0.0f;

	pMatrix->m[2][0] = m_vRightVector.z;
	pMatrix->m[2][1] = m_vUpVector.z;
	pMatrix->m[2][2] = m_vLookVector.z;
	pMatrix->m[2][3] = 0.0f;

	pMatrix->m[3][0] = -vector_dotproduct(&m_vRightVector, &m_vCameraPosition);		//-P*R
	pMatrix->m[3][1] = -vector_dotproduct(&m_vUpVector, &m_vCameraPosition);	    //-P*U
	pMatrix->m[3][2] = -vector_dotproduct(&m_vLookVector, &m_vCameraPosition);		//-P*L
	pMatrix->m[3][3] = 1.0f;

}

void Camera::SetTargetPosition(vector_t* pLookat)
{
	vector_t vecTemp;
	vecTemp.x = 0.0f; vecTemp.y = 0.0f; vecTemp.z = 1.0f;
	if (pLookat != NULL) m_vTargetPosition = (*pLookat);
	else m_vTargetPosition = vecTemp;


	vector_sub(&m_vLookVector, &m_vTargetPosition, &m_vCameraPosition);
	vector_normalize(&m_vLookVector);

	vector_crossproduct(&m_vUpVector, &m_vLookVector, &m_vRightVector);
	vector_normalize(&m_vUpVector);

	vector_crossproduct(&m_vRightVector, &m_vUpVector, &m_vLookVector);
	vector_normalize(&m_vRightVector);
}

void Camera::SetCameraPosition(vector_t* pVector)
{
	vector_t v;
	v.x = 0.0f;
	v.y = 0.0f;
	v.z = -250.0f;
	v.w = 0.0f;
	m_vCameraPosition = pVector ? (*pVector) : v;
}

void Camera::SetViewMatrix(matrix_t* pMatrix)
{
	if (pMatrix)m_matView = *pMatrix;
	else CalculateViewMatrix(&m_matView);

	m_vRightVector.x = m_matView.m[0][0]; m_vRightVector.y = m_matView.m[0][1]; m_vRightVector.z = m_matView.m[0][2];
	m_vUpVector.x = m_matView.m[1][0]; m_vUpVector.y = m_matView.m[1][1]; m_vUpVector.z = m_matView.m[1][2];
	m_vLookVector.x = m_matView.m[2][0]; m_vLookVector.y = m_matView.m[2][1]; m_vLookVector.z = m_matView.m[2][2];
}

//Œ¥ µœ÷
void Camera::SetProjMatrix(matrix_t* pMatrix)
{
	if (pMatrix != NULL) m_matPorj = *pMatrix;
	else
	{

	}
}

void Camera::MoveAlongRightVec(float fUnits)
{
	vector_t tempVec;
	tempVec.x = m_vRightVector.x * fUnits;
	tempVec.y = m_vRightVector.y * fUnits;
	tempVec.z = m_vRightVector.z * fUnits;
	
	vector_add(&m_vCameraPosition, &m_vCameraPosition, &tempVec);
	vector_add(&m_vTargetPosition, &m_vTargetPosition, &tempVec);
}

void Camera::MoveAlongUpVec(float fUnits)
{
	vector_t tempVec;
	tempVec.x = m_vUpVector.x * fUnits;
	tempVec.y = m_vUpVector.y * fUnits;
	tempVec.z = m_vUpVector.z * fUnits;

	vector_add(&m_vCameraPosition, &m_vCameraPosition, &tempVec);
	vector_add(&m_vTargetPosition, &m_vTargetPosition, &tempVec);
}

void Camera::MoveAlongLookVec(float fUnits)
{
	vector_t tempVec;
	tempVec.x = m_vLookVector.x * fUnits;
	tempVec.y = m_vLookVector.y * fUnits;
	tempVec.z = m_vLookVector.z * fUnits;

	vector_add(&m_vCameraPosition, &m_vCameraPosition, &tempVec);
	vector_add(&m_vTargetPosition, &m_vTargetPosition, &tempVec);
}

void Camera::RotationRightVec(float fAngle)
{
	matrix_t R;
	matrix_set_rotate(&R, m_vRightVector.x, m_vRightVector.y, m_vRightVector.z, fAngle);
	vector_TransformCoord(&m_vRightVector, &m_vRightVector, &R);
	vector_TransformCoord(&m_vLookVector, &m_vLookVector, &R);

	//cout << m_vRightVector.x << " " << m_vRightVector.y << " " << m_vRightVector.z << " " << m_vRightVector.w << endl;

	vector_scale(&m_vTargetPosition, &m_vLookVector, vector_length(&m_vCameraPosition));
}

void Camera::RotationUpVec(float fAngle)
{
	matrix_t R;
	matrix_set_rotate(&R, m_vUpVector.x,m_vUpVector.y,m_vUpVector.z, fAngle);
	vector_TransformCoord(&m_vRightVector, &m_vRightVector, &R);
	vector_TransformCoord(&m_vLookVector, &m_vLookVector, &R);

	vector_scale(&m_vTargetPosition, &m_vLookVector, vector_length(&m_vCameraPosition));
}

void Camera::RotationLookVec(float fAngle)
{
	matrix_t R;
	matrix_set_rotate(&R, m_vLookVector.x, m_vLookVector.y, m_vLookVector.z, fAngle);
	vector_TransformCoord(&m_vRightVector, &m_vRightVector, &R);
	vector_TransformCoord(&m_vLookVector, &m_vLookVector, &R);

	vector_scale(&m_vTargetPosition, &m_vLookVector, vector_length(&m_vCameraPosition));
}

Camera::~Camera(void)
{
}