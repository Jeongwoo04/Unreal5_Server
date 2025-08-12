#pragma once
#include <random>

#define PI 3.1415926f

static const float CELL_SIZE = 100.f;

struct Vector2Int
{
    int32 _x = 0;
    int32 _y = 0;

    Vector2Int() = default;
    Vector2Int(int32 x, int32 y) : _x(x), _y(y) { }
    Vector2Int(const Protocol::PosInfo& pos)
    {
        _x = static_cast<int32>(round(pos.x() / CELL_SIZE));
        _y = static_cast<int32>(round(pos.y() / CELL_SIZE));
    }

    Vector2Int& operator=(const Vector2Int& other)
    {
        _x = other._x; _y = other._y;
        return *this;
    }

    Vector2Int operator+(const Vector2Int& other) const
    {
        return  Vector2Int(_x + other._x, _y + other._y);
    }

    Vector2Int& operator+=(const Vector2Int& other)
    {
        _x += other._x;
        _y += other._y;
        return *this;
    }

    Vector2Int operator-(const Vector2Int& other) const
    {
        return Vector2Int(_x - other._x, _y - other._y);
    }

    Vector2Int& operator-=(const Vector2Int& other)
    {
        _x -= other._x;
        _y -= other._y;
        return *this;
    }

    bool operator==(const Vector2Int& other) const
    {
        return (_x == other._x && _y == other._y);
    }

    float magnitude() const
    {
        return std::sqrt(static_cast<float>(sqrMagnitude()));
    }
    int32 sqrMagnitude() const
    {
        return _x * _x + _y * _y;
    }
    int32 cellDist() const
    {
        return std::abs(_x) + std::abs(_y);
    }

    // Debug Ãâ·Â
    friend std::ostream& operator<<(std::ostream& os, const Vector2Int& vec)
    {
        os << "(" << vec._x << ", " << vec._y << ")";
        return os;
    }
};

struct Vector3
{
    float _x, _y;
    Vector3() { };
    Vector3(float x, float y) : _x(x), _y(y) { }
    Vector3(const Protocol::PosInfo& pos) { _x = pos.x(); _y = pos.y(); }

    Vector3 operator-(const Vector3& other)
    {
        return Vector3(_x - other._x, _y - other._y);
    }

    Vector3 operator+(const Vector3& other) const
    {
        return Vector3(_x + other._x, _y + other._y);
    }

    Vector3& operator+=(const Vector3& other)
    {
        _x += other._x;
        _y += other._y;
        return *this;
    }

    Vector3 operator*(float scalar) const
    {
        return Vector3(_x * scalar, _y * scalar);
    }

    Vector3 operator/(float scalar) const
    {
        return Vector3(_x / scalar, _y / scalar);
    }

    float Length() const
    {
        return sqrtf(_x * _x + _y * _y);
    }

    float LengthSquared() const
    {
        return _x * _x + _y * _y;
    }

    Vector3 Normalized() const
    {
        float len = sqrtf(_x * _x + _y * _y);
        if (len == 0)
            return Vector3{ 0, 0 };
        return Vector3{ _x / len, _y / len };
    }

    static Vector3 YawToDir(float yaw)
    {
        float radians = yaw * (PI / 180.f);
        float x = cosf(radians);
        float y = sinf(radians);

        return Vector3(x, y).Normalized();
    }

    static float Dot(const Vector3& a, const Vector3& b)
    {
        return a._x * b._x + a._y * b._y;
    }
};

static Vector2Int WorldToGrid(const Vector3& vec, float CELL_SIZE = 100.f)
{
    return Vector2Int(
        static_cast<int32>(round(vec._x / CELL_SIZE)),
        static_cast<int32>(round(vec._y / CELL_SIZE))
    );
}
static Vector3 GridToWorld(const Vector2Int& vec)
{
    return Vector3(vec._x * CELL_SIZE, vec._y * CELL_SIZE);
}

struct Vector2IntHash
{
    size_t operator()(const Vector2Int& vec) const noexcept
    {
        size_t h1 = hash<int32>()(vec._x);
        size_t h2 = hash<int32>()(vec._y);
        return h1 ^ (h2 << 1);
    }
};

class Utils
{
public:
	template<typename T>
	static T GetRandom(T min, T max)
	{
		std::random_device randomDevice;
		std::mt19937 generator(randomDevice());

		if constexpr (std::is_integral_v<T>)
		{
			std::uniform_int_distribution<T> distribution(min, max);
			return distribution(generator);
		}
		else
		{
			std::uniform_real_distribution<T> distribution(min, max);
			return distribution(generator);
		}
	}
};

static float Clamp01(float value)
{
    return max(0.f, min(1.f, value));
}

static bool CheckCapsuleHitWithT(Vector3& from, Vector3& to, Vector3& center, float radius, OUT float& T)
{
    Vector3 ab = to - from;
    Vector3 ap = center - from;

    float abLengthSquared = ab.LengthSquared();
    if (abLengthSquared == 0.f)
    {
        T = 0.f;
        return (center - from).LengthSquared() <= radius * radius;
    }

    float t = Clamp01(Vector3::Dot(ap, ab) / abLengthSquared);
    Vector3 close = from + ab * t;

    float distSquared = (center - close).LengthSquared();
    if (distSquared <= radius * radius)
    {
        T = t;
        return true;
    }

    return false;
}