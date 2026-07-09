#include "Camera.h"

Camera::Camera()
{
}

void Camera::SetPosition(float x, float y)
{
    m_X = x;
    m_Y = y;
}

void Camera::Move(float dx, float dy)
{
    m_X += dx;
    m_Y += dy;
}

void Camera::SetZoom(float zoom)
{
    m_Zoom = zoom;
}

float Camera::GetX() const
{
    return m_X;
}

float Camera::GetY() const
{
    return m_Y;
}

float Camera::GetZoom() const
{
    return m_Zoom;
}