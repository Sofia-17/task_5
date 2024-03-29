#include <cmath>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <float.h>
#include <fstream>
#include "CImg.h"
using namespace std;

struct Color {
    int red;
    int green;
    int blue;
    inline Color(int r = 0, int g = 0, int b = 0) {
        red = r;
        green = g;
        blue = b;
    }
};

#define PI 3.1415926

double xl = 0, yl = 0, zl = 0;
double maxDist = 0;
double minDist = DBL_MAX;

#define SPECULARITY 10

class Vector3 {
public:
	double x, y, z;
	Vector3(double x = 0, double y = 0, double z = 0) {
		this->x = x;
		this->y = y;
		this->z = z;
	}
	Vector3 normalize() {
		double r = sqrt(x * x + y * y + z * z);
		return Vector3(x / r, y / r, z / r);
	}
	double operator*(const Vector3& r) {
		double touch = 0;
		touch += x * r.x + y * r.y + z * r.z;
		return touch;
	}
	Vector3 operator*(double r) {
	    return {x*r,y*r,z*r};
	}
	Vector3 operator-(const Vector3& r) {
		return Vector3(x - r.x,y - r.y,z - r.z);
	}
	Vector3 operator+(const Vector3& r) {
		return Vector3(x + r.x, y + r.y, z + r.z);
	}
	double length() const {
		return sqrt(x * x + y * y + z * z);
	}
	bool operator>(const Vector3& r) {
		if (length() > r.length()) {
			return true;
		}
		return false;
	}
	Vector3 crossProduct(Vector3 b) {
		Vector3 c;
		c.x = y * b.z - z * b.y;
		c.y = z * b.x - x * b.z;
		c.z = x * b.y - y * b.x;
		return c;
	}
	Vector3 operator-() {
		return Vector3(-x, -y, -z);
	}
};

class Object {
protected:
	Vector3 v0;
public:
	Color col;
	virtual bool intersect(double, double, double, double, double, double, double&, bool&) = 0;
	virtual Color pixelColor(double, double, double) = 0;
	virtual void setColor(Color) = 0;
	virtual Vector3 getCenter() = 0;
};

class Sphere : public Object {
	double R;
public:
	Sphere(Vector3 v0, double R, Color col = Color(255,0,0)) {
		this->v0 = v0;
		this->R = R;
		this->col = col;
	}
	bool intersect(double A, double B, double C, double xc, double yc, double zc, double& touch, bool& tch) override {
		double d1 = ((v0.x-xc) * A / B + (v0.y-yc) + (v0.z-zc) * C / B) * ((v0.x-xc) * A / B + (v0.y-yc) + (v0.z-zc) * C / B) - (A * A / (B * B) + 1 + C * C / (B * B)) * ((v0.x-xc) * (v0.x-xc) + (v0.y-yc) * (v0.y-yc) + (v0.z-zc) * (v0.z-zc) - R * R);
		if (d1 >= 0) {
			double y1 = (((v0.x-xc) * A / B + (v0.y-yc) + (v0.z-zc) * C / B) + sqrt(d1)) / (A * A / (B * B) + 1 + C * C / (B * B));
			double y2 = (((v0.x-xc) * A / B + (v0.y-yc) + (v0.z-zc) * C / B) - sqrt(d1)) / (A * A / (B * B) + 1 + C * C / (B * B));
			if (d1 == 0) {
				tch = true;
			} else {
				tch = false;
			}
			touch = min(y1, y2);
			return true;
		}
		touch = 0;
		tch = 0;
		return false;
	}
	Color pixelColor(double x, double y, double z) override {
		Vector3 n(2 * (x - v0.x), 2 * (y - v0.y), 2 * (z - v0.z));
		n = n.normalize();
		Vector3 a(xl - x, yl - y, zl - z);
		a = a.normalize();
		Color c = col;
		double light = n * a;
        Vector3 gloss = (n * (n * a)) * 2 - a;
        gloss = gloss.normalize();
        double glossiness = - (gloss * Vector3(x,y,z).normalize());
        if (glossiness < 0) {
            glossiness = 0;
        }
        glossiness = pow(glossiness, SPECULARITY);
        if (light < 0) {
            light = 0;
            glossiness = 0;
        }
		c.red = static_cast<int>(min(c.red * light + 10 + 255*glossiness,255.0));
		c.green = static_cast<int>(min(c.green * light + 10+255*glossiness, 255.0));
		c.blue = static_cast<int>(min(c.blue * light + 10+255*glossiness, 255.0));
		return c;
	}
	void setColor(Color c) override {
	    col = c;
	}
	Vector3 getCenter() override {
	    return v0;
	}
};

class Treygolnik : public Object {
	Vector3 v1;
	Vector3 v2;
	double Ap, Bp, Cp, Dp;
public:
	Treygolnik(Vector3 v0, Vector3 v1, Vector3 v2, Color col = Color(255, 0, 0)) {
		this->v0 = v0;
		this->v1 = v1;
		this->v2 = v2;
		this->col = col;
		Ap = (v1.y - v0.y) * (v2.z - v0.z) - (v1.z - v0.z) * (v2.y - v0.y);
		Bp = (v1.z - v0.z) * (v2.x - v0.x) - (v1.x - v0.x) * (v2.z - v0.z);
		Cp = (v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x);
		Dp = -Ap * v0.x - Bp * v0.y - Cp * v0.z;
	}
	bool intersect(double A, double B, double C, double xc, double yc, double zc, double& touch, bool& tch) override {
		double nozir = (Ap * A + Bp * B + Cp * C);
		if (abs(nozir) < DBL_MIN) {
			return false;
		}
		double t = - Dp / nozir;
		double xo = A * t;
		double yo = B * t;
		double zo = C * t;
		Vector3 o(xo, yo, zo);
		Vector3 a(v0.x, v0.y, v0.z);
		Vector3 b(v1.x, v1.y, v1.z);
		Vector3 c(v2.x, v2.y, v2.z);
		Vector3 n1 = (o - a).crossProduct(b - a);
		Vector3 n2 = (o - b).crossProduct(c - b);
		Vector3 n3 = (o - c).crossProduct(a - c);
		if (n1 * n2 < 0 || n1 * n3 < 0 || n2 * n3 < 0) {
			touch = 0;
			tch = false;
			return false;
		}
		touch = yo;
		tch = false;
		return true;
	}
	Color pixelColor(double x, double y, double z) override {
		Vector3 n(Ap, Bp, Cp);
		n = n.normalize();
		Vector3 a(xl - x, yl - y, zl - z);
		if (a * n < 0) {
			n = -n;
		}
		a = a.normalize();
		Color c = col;
		double light = n * a;
		if (light < 0) {
			light = 0;
		}
		c.red = static_cast<int>(min(c.red * light + 10, 255.0));
		c.green = static_cast<int>(min(c.green * light + 10, 255.0));
		c.blue = static_cast<int>(min(c.blue * light + 10, 255.0));
		return c;
	}
	void setColor(Color c) override {
	    col = c;
	}
	Vector3 getCenter() override {
	    Vector3 center;
        center.x = min(min(v0.x,v1.x),v2.x) + (max(max(v0.x,v1.x),v2.x) - min(min(v0.x,v1.x),v2.x))/2;
        center.y = min(min(v0.y,v1.y),v2.y) + (max(max(v0.y,v1.y),v2.y) - min(min(v0.y,v1.y),v2.y))/2;;
        center.z = min(min(v0.z,v1.z),v2.z) + (max(max(v0.z,v1.z),v2.z) - min(min(v0.z,v1.z),v2.z))/2;;
        return center;
	}
};

class Cube : public Object {
	vector<Treygolnik*> s;
	int c = 0;
public:
    Cube(Vector3 center, Vector3 v0,Vector3 v1,Vector3 v2,Vector3 v3, double a) {
        this->v0 = center;
		Vector3 point2 = v0 - v1 * a;
		Vector3 point3 = v0 - v2 * a;
		Vector3 point4 = v0 - v3 * a;
		Vector3 point5 = v0 - v1 * a - v2 * a;
		Vector3 point6 = v0 - v1 * a - v3 * a;
		Vector3 point7 = v0 - v2 * a - v3 * a;
		Vector3 point8 = v0 - v1 * a - v2 * a - v3 * a;
        s.push_back(new Treygolnik(v0, point2, point3));
        s.push_back(new Treygolnik(point3, point2, point5));

		s.push_back(new Treygolnik(point7, point4, point6));
		s.push_back(new Treygolnik(point7, point6, point8));

		s.push_back(new Treygolnik(point3, v0, point4));
		s.push_back(new Treygolnik(point3, point4, point7));

		s.push_back(new Treygolnik(point5, point2, point6));
		s.push_back(new Treygolnik(point5, point6, point8));

		s.push_back(new Treygolnik(point3, point7, point5));
		s.push_back(new Treygolnik(point7, point8, point5));

		s.push_back(new Treygolnik(v0, point4, point2));
		s.push_back(new Treygolnik(point4, point2, point6));
    }
    bool intersect(double A, double B, double C, double xc, double yc, double zc, double &touch, bool &tch) override {
		double cubeymax = DBL_MAX;
		bool found = false;
		double touch1 = 0;
		double tch1 = 0;
		for (int i = 0; i < s.size(); i++) {
			if (s[i]->intersect(A, B, C, xc, yc, zc, touch, tch)) {
				if (touch < cubeymax) {
					cubeymax = touch;
					c = i;
					found = true;
					touch1 = touch;
					tch1 = tch;
				}
			}
		}
		touch = touch1;
		tch = tch1;
		return found;
    }
    Color pixelColor(double x, double y, double z) override {
        return s[c]->pixelColor(x,y,z);
    }
    void setColor(Color c) override {
        col = c;
		for (int i = 0; i < s.size(); i++) {
			s[i]->col = c;
		}
    }
    Vector3 getCenter() override {
        return v0;
    }
};

int main() {
	vector<Object*> objs;
	double fov;
	int x, y;
	ifstream in("conf.txt");
	in >> fov >> x >> y;
	if (x % 2 != 0) {
		x++;
	}
	if (y % 2 != 0) {
		y++;
	}
    cimg_library::CImg<unsigned char> image(x,y,1,3,0);
    cimg_library::CImgDisplay display(image, "raytracing");
	double scry = sqrt(static_cast<double>(x * x) / (2 * (1 - cos(fov / 180 * PI))) - static_cast<double>(x * x) / 4);
	in >> xl >> yl >> zl;
	while (!in.eof()) {
		std::string name;
		in >> name;
		if (name == "sphere") {
			double x0, y0, z0, r;
			in >> x0 >> y0 >> z0 >> r;
			Vector3 v0 = Vector3(x0,y0,z0);
			if (v0.length() > maxDist) {
			    maxDist = v0.length();
            }
			if (v0.length() < minDist) {
			    minDist = v0.length();
			}
			objs.push_back(new Sphere(v0, r));
		}
		else if (name == "cube") {
			double x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, a;
			in >> x1 >> y1 >> z1 >> x2 >> y2 >> z2 >> x3 >> y3 >> z3 >> x4 >> y4 >> z4 >> a;
			Vector3 v0;
			v0 = Vector3(x1, y1, z1) - Vector3(x2, y2, z2) * (a / 2) - Vector3(x3, y3, z3) * (a / 2) - Vector3(x4, y4, z4) * (a / 2);
            if (v0.length() > maxDist) {
                maxDist = v0.length();
            }
            if (v0.length() < minDist) {
                minDist = v0.length();
            }
			objs.push_back(new Cube(v0,Vector3(x1, y1, z1), Vector3(x2, y2, z2), Vector3(x3, y3, z3), Vector3(x4,y4,z4),a));
		}
	}
	for (Object* o : objs) {
        double shade;
        if (maxDist == minDist) {
            shade = 0;
        } else {
            shade = ((255.0 / (maxDist - minDist))) * o->getCenter().length() -
                             (255.0 * minDist) / (maxDist - minDist);
        }
        if (shade < 0) shade = 0;
		if (shade > 255) shade = 255;
        Color c(255 - shade, shade, 0);
        o->setColor(c);
	}
	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
#pragma omp parallel for
	for (int i = -y / 2; i < y / 2; i++) {
#pragma omp parallel for
		for (int j = -x / 2; j < x / 2; j++) {
			bool pixelSet = false;
			double cury = DBL_MAX;
			for (Object* o : objs) {
				double yx;
				bool tch;
				if (o->intersect(j, scry, i, 0, 0, 0, yx, tch)) {
					if (yx < cury) {
						cury = yx;
						Color c = o->pixelColor(static_cast<double>(j) * yx / scry, yx, static_cast<double>(i) * yx / scry);
						unsigned char color[3] = { (unsigned char)c.red,(unsigned char)c.green,(unsigned char)c.blue };
						image.draw_point(x / 2 + j, y / 2 + i, color);
						pixelSet = true;
					}
				}
			}
			if (!pixelSet) {
                unsigned char color[3] = {(unsigned char)0,(unsigned char)0,(unsigned char)0};
                image.draw_point(x / 2 + j, y / 2 + i, color);
			}
		}
	}
	std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
	int elapsed_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
	std::cout << "Render took: " << elapsed_ms << " ms\n";
    display.display(image);
    image.save("test.bmp");
	while (!display.is_closed())
        display.wait();
}
