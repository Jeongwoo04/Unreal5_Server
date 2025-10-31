#pragma once
#include <random>

#define PI 3.1415926f

static const float CELL_SIZE = 100.f;

struct Vector3;

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
    float _x, _y, _z;
    Vector3() { };
    Vector3(const Protocol::Vec3& targetPos) { _x = targetPos.x(); _y = targetPos.y(); _z = targetPos.z(); }
    Vector3(float x, float y, float z) : _x(x), _y(y), _z(z) { }
    Vector3(const Protocol::PosInfo& pos) { _x = pos.x(); _y = pos.y(); _z = pos.z(); }

    Vector3 operator-(const Vector3& other) const
    {
        return Vector3(_x - other._x, _y - other._y, _z - other._z);
    }

    Vector3 operator+(const Vector3& other) const
    {
        return Vector3(_x + other._x, _y + other._y, _z + other._z);
    }

    Vector3& operator+=(const Vector3& other)
    {
        _x += other._x;
        _y += other._y;
        _z += other._z;
        return *this;
    }

    bool operator==(const Vector3& other)
    {
        return _x == other._x && _y == other._y && _z == other._z;
    }

    bool operator!=(const Vector3& other)
    {
        return *this == other;
    }

    Vector3 operator+(float scalar) const
    {
        return Vector3(_x + scalar, _y + scalar, _z + scalar);
    }

    Vector3 operator*(float scalar) const
    {
        return Vector3(_x * scalar, _y * scalar, _z * scalar);
    }

    Vector3 operator/(float scalar) const
    {
        return Vector3(_x / scalar, _y / scalar, _z / scalar);
    }

    float LengthSquared2D() const { return _x * _x + _y * _y; }
    float Length2D() const { return sqrtf(LengthSquared2D()); }

    float LengthSquared() const { return _x * _x + _y * _y + _z * _z; }
    float Length() const { return sqrtf(LengthSquared()); }

    Vector3 Normalized2D() const
    {
        float len = Length2D();
        if (len <= FLT_EPSILON)
            return Vector3{ 0.f, 0.f, 0.f };
        return Vector3{ _x / len, _y / len, 0.f };
    }

    Vector3 Normalized() const
    {
        float len = Length();
        if (len <= FLT_EPSILON)
            return Vector3{ 0.f, 0.f, 0.f };
        return Vector3{ _x / len, _y / len, _z / len };
    }

    static Vector3 YawToDir2D(float yaw)
    {
        float radians = yaw * (PI / 180.f);
        float x = cosf(radians);
        float y = sinf(radians);

        return Vector3(x, y, 0).Normalized2D();
    }

    static float DirToYaw2D(const Vector3& dir)
    {
        float radian = atan2(dir._y, dir._x);
        float degree = radian * 180.0f / PI;
        return degree;
    }

    static float Dot2D(const Vector3& a, const Vector3& b)
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
    return Vector3(vec._x * CELL_SIZE, vec._y * CELL_SIZE, 0);
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

static double GetTimeMs()
{
    using namespace std::chrono;
    auto now = steady_clock::now();
    return duration_cast<microseconds>(now.time_since_epoch()).count() / 1000.0;
}

static float Clamp01(float value)
{
    return max(0.f, min(1.f, value));
}

static bool CheckCapsuleHitWithT(Vector3& from, Vector3& to, Vector3& center, float radius, OUT float& T)
{
    Vector3 ab = to - from;
    Vector3 ap = center - from;

    float abLengthSquared = ab.LengthSquared2D();
    if (abLengthSquared == 0.f)
    {
        T = 0.f;
        return (center - from).LengthSquared2D() <= radius * radius;
    }

    float t = Clamp01(Vector3::Dot2D(ap, ab) / abLengthSquared);
    Vector3 close = from + ab * t;

    float distSquared = (center - close).LengthSquared2D();
    if (distSquared <= radius * radius)
    {
        T = t;
        return true;
    }

    return false;
}