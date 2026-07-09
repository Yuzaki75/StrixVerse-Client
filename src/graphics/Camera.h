#pragma once

class Camera
{
public:
    Camera();

    void SetPosition(
        float x,
        float y);

    void Move(
        float dx,
        float dy);

    void SetZoom(
        float zoom);

    float GetX() const;

    float GetY() const;

    float GetZoom() const;

private:
    float m_X = 0.0f;
    float m_Y = 0.0f;

    float m_Zoom = 1.0f;
};