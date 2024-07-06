#pragma once
#include <Windows.h>
#include <cmath>

extern int screenWidth;
extern int screenHeight;

struct view_matrix_t {
	float* operator[ ](int index) {
		return matrix[index];
	}
	float matrix[4][4];
};

struct Vector2 {

	constexpr Vector2(
		const float x = 0.f,
		const float y = 0.f) noexcept :
		x(x), y(y) { }

	constexpr const Vector2& operator-(const Vector2& other) const noexcept
	{
		return Vector2{ x - other.x, y - other.y };
	}
	constexpr const Vector2& operator+(const Vector2& other) const noexcept
	{
		return Vector2{ x + other.x, y + other.y };
	}
	constexpr const Vector2& operator/(const float factor) const noexcept
	{
		return Vector2{ x / factor, y / factor };
	}
	constexpr const Vector2& operator*(const float factor) const noexcept
	{
		return Vector2{ x * factor, y * factor };
	}
	static float Distance(const Vector2& first, const Vector2& second)
	{
		return sqrt(pow(first.x - second.x, 2) + pow(first.y - second.y, 2));
	}

	Vector2 WTF(view_matrix_t matrix) const
	{
		float _x = matrix[0][0] * x + matrix[0][1] * y + matrix[0][3];
		float _y = matrix[1][0] * x + matrix[1][1] * y + matrix[1][3];

		float w = matrix[3][0] * x + matrix[3][1] * y + matrix[3][3];

		if (w < 0.01f)
			return false;

		float inv_w = 1.f / w;
		_x *= inv_w;
		_y *= inv_w;

		float x = screenWidth / 2;
		float y = screenHeight / 2;

		x += 0.5f * _x * screenWidth + 0.5f;
		y -= 0.5f * _y * screenHeight + 0.5f;

		return { x,y };

	}

	float x, y;

};

struct Vector3 {
	// constructor
	constexpr Vector3(
		const float x = 0.f,
		const float y = 0.f,
		const float z = 0.f) noexcept :
		x(x), y(y), z(z) { }

	// operator overloads
	constexpr const Vector3& operator-(const Vector3& other) const noexcept
	{
		return Vector3{ x - other.x, y - other.y, z - other.z };
	}
	constexpr const Vector3& operator+(const Vector3& other) const noexcept
	{
		return Vector3{ x + other.x, y + other.y, z + other.z };
	}
	constexpr const Vector3& operator/(const float factor) const noexcept
	{
		return Vector3{ x / factor, y / factor, z / factor };
	}
	constexpr const Vector3& operator*(const float factor) const noexcept
	{
		return Vector3{ x * factor, y * factor, z * factor };
	}

	static Vector3 Lerp(const Vector3& first, const Vector3& second, float time)
	{
		return Vector3(first.x + time * (second.x - first.x), first.y + time * (second.y - first.y), first.z + time * (second.z - first.z));
	}

	static Vector2 CalculateAngles(Vector3 from, Vector3 to)
	{
		float yaw;
		float pitch;

		float deltaX = to.x - from.x;
		float deltaY = to.y - from.y;
		float deltaZ = to.z - from.z;

		double distance = sqrt(pow(deltaX, 2) + pow(deltaY, 2));

		yaw = atan2(deltaY, deltaX) * 180 / 3.14;
		pitch = -(atan2(deltaZ, distance) * 180 / 3.14);

		return Vector2(yaw, pitch);
	}

	Vector3 WTS(view_matrix_t matrix) const
	{
		float _x = matrix[0][0] * x + matrix[0][1] * y + matrix[0][2] * z + matrix[0][3];
		float _y = matrix[1][0] * x + matrix[1][1] * y + matrix[1][2] * z + matrix[1][3];

		float w = matrix[3][0] * x + matrix[3][1] * y + matrix[3][2] * z + matrix[3][3];

		if (w < 0.01f)
			return false;

		float inv_w = 1.f / w;
		_x *= inv_w;
		_y *= inv_w;

		float x = screenWidth / 2;
		float y = screenHeight / 2;

		x += 0.5f * _x * screenWidth + 0.5f;
		y -= 0.5f * _y * screenHeight + 0.5f;

		return { x,y,w };
	}

	float x, y, z;
};

struct Vector4 {

	constexpr Vector4(
		const float x = 0.f,
		const float y = 0.f,
		const float z = 0.f,
		const float w = 0.f) noexcept :
		x(x), y(y), z(z), w(w) { }

	constexpr const Vector4& operator-(const Vector4& other) const noexcept
	{
		return Vector4{ x - other.x, y - other.y, z - other.z, w - other.w };
	}
	constexpr const Vector4& operator+(const Vector4& other) const noexcept
	{
		return Vector4{ x + other.x, y + other.y, z + other.z, w + other.w };
	}
	constexpr const Vector4& operator/(const float factor) const noexcept
	{
		return Vector4{ x / factor, y / factor, z / factor, w / factor };
	}
	constexpr const Vector4& operator*(const float factor) const noexcept
	{
		return Vector4{ x * factor, y * factor, z * factor, w * factor };
	}

	float x, y, z, w;
};
