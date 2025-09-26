#pragma once

#include "Utils.h"
#include <algorithm>

class GeometryUtil
{
public:
    // 원형 범위
    template<typename T>
    static vector<T> FindInCircle2D(const vector<T>& candidates,
        const Vector3& center, float radius)
    {
        vector<T> result;
        float radiusSq = radius * radius;
        for (T obj : candidates)
        {
            if (!obj)
                continue;
            Vector3 pos(obj->_posInfo);
            float distSq = (pos - center).LengthSquared2D();
            if (distSq <= radiusSq)
                result.push_back(obj);
        }
        return result;
    }

    // 전방 부채꼴 (시야각 fov, 거리 range)
    template<typename T>
    static vector<T> FindInCone2D(const vector<T>& candidates,
        const Vector3& center, const Vector3& forward,
        float fovDeg, float range)
    {
        vector<T> result;
        float cosHalfFov = cosf((fovDeg * 0.5f) * (PI / 180.f));
        float rangeSq = (range * CELL_SIZE) * (range * CELL_SIZE);

        Vector3 forward2D = forward.Normalized2D();

        for (T obj : candidates)
        {
            if (!obj) continue;
            Vector3 dir = Vector3(obj->_posInfo) - center;
            float distSq = dir.LengthSquared2D();
            if (distSq > rangeSq)
                continue;

            dir.Normalized2D();
            float dot = Vector3::Dot2D(forward2D, dir);
            if (dot >= cosHalfFov) // 전방 시야각 안에 들어옴
                result.push_back(obj);
        }
        return result;
    }

    // 직사각형 (폭 width, 길이 length)
    template<typename T>
    static vector<T> FindInRectangle2D(const vector<T>& candidates,
        const Vector3& center, const Vector3& forward,
        float width, float length)
    {
        vector<T> result;

        Vector3 forward2D = forward.Normalized2D();
        Vector3 right(-forward2D._y, forward2D._x, 0.f);

        for (T obj : candidates)
        {
            if (!obj) continue;

            Vector3 dir = Vector3(obj->_posInfo) - center;

            float distForward = Vector3::Dot2D(forward2D, dir);
            float distRight = Vector3::Dot2D(right, dir);

            if (distForward >= 0 && distForward <= length * CELL_SIZE &&
                fabs(distRight) <= (width * CELL_SIZE * 0.5f))
            {
                result.push_back(obj);
            }
        }
        return result;
    }

    // 선/직선 판정 (광선)
    template<typename T>
    static vector<T> FindInLine2D(const vector<T>& candidates,
        const Vector3& from, const Vector3& to, float radius)
    {
        vector<T> result;
        for (T obj : candidates)
        {
            if (!obj) continue;

            Vector3 objPos(obj->_posInfo);
            float totalRadius = radius + obj->_collisionRadius;

            if (CheckCapsuleHit2D(from, to, objPos, totalRadius))
                result.push_back(obj);
        }
        return result;
    }

private:
    // 캡슐 충돌 체크
    static bool CheckCapsuleHit2D(const Vector3& from, const Vector3& to,
        const Vector3& center, float radius)
    {
        Vector3 ab = to - from;
        Vector3 ap = center - from;
        float t = Clamp01(Vector3::Dot2D(ap, ab) / ab.LengthSquared2D());

        Vector3 close = from + ab * t;
        float distSquared = (center - close).LengthSquared2D();
        return distSquared <= (radius * radius);
    }
};

