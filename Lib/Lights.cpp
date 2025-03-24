// Lights.cpp - representation/operations on lights

#include "Draw.h"
#include "Lights.h"
#include <stdio.h>

vec3 palette[] = {vec3(1,0,0), vec3(0,1,0), vec3(0,0,1), vec3(1,1,0), vec3(1,0.6f,0), vec3(1,0,1), vec3(0,1,1), vec3(1,1,1)};
int npalette = sizeof(palette)/sizeof(vec3);

vector<vec3> *Lights::Transform(mat4 view) {
	xformedPoints.resize(points->size());
	for (vec3 &v : xformedPoints)
		v = Vec3(view*vec4(v, 1));
	return &xformedPoints;
}
/*
int Lights::Transform(mat4 view, vector<vec3> &xLights) {
	int nlights = lights.size();
	xLights.resize(nlights);
	for (int i = 0; i < nlights; i++)
		xLights[i] = Vec3(view*vec4(lights[i].p, 1));
	return nlights;
}

void Lights::TransformSetColors(mat4 view, vector<vec3> &xLights, vector<vec3> &colors) {
	Transform(view, xLights);
	int nlights = lights.size();
	colors.resize(nlights);
	for (int i = 0; i < nlights; i++) {
		Light l = lights[i];
		colors[i] = palette[cids[i]]; //  >= npalette? npalette-1 : l.cid];
	}
}
*/
void Lights::Display(mat4 fullview, vec3 hiddenColor, vec3 *hover) {
	for (size_t i = 0; i < points->size(); i++) {
		vec3 *l = &(*points)[i];
		float dia = hover == l? 12.f : 9.f;
		Star(*l, dia, palette[cids[i]], hiddenColor);
	}
}

vec3 *Lights::MouseOver(int x, int y, mat4 fullview) {
	for (size_t i = 0; i < points->size(); i++)
		if (::MouseOver(x, y, (*points)[i], fullview, 12))
			return &(*points)[i];
	return NULL;
}

bool Lights::Down(int x, int y, mat4 modelview, mat4 persp) {
	vec3 *l = MouseOver(x, y, persp*modelview);
	picked = NULL;
	if (l) {
		picked = l;
		mover.Down(l, x, y, modelview, persp);
		return true;
	}
	return false;
}

void Lights::Drag(int x, int y, mat4 modelview, mat4 persp) {
	mover.Drag(x, y, modelview, persp);
}

void Lights::AddLight() {
	if (picked) {
		int i = picked-points->data();
		size_t nlights = points->size();
		points->resize(nlights+1);
		(*points)[nlights] = *picked;
		cids.resize(nlights+1);
		cids[nlights] = cids[i];
		picked = points->data()+nlights;
	}
}

void Lights::DeleteLight() {
	if (picked) {
		size_t id = picked-points->data();
		points->erase(points->begin()+id);
		cids.erase(cids.begin()+id);
		picked = NULL;
	}
}

void Lights::ChangeColor() {
	if (picked) {
		int id = picked-points->data();
		cids[id] = (cids[id]+1) % npalette;
	}
}

void Lights::Save(const char *filename) {
	int nlights = points->size();
	FILE *out = fopen(filename, "wb");
	fwrite(&nlights, sizeof(int), 1, out);
	for (int i = 0; i < nlights; i++)
		fwrite(points+i, sizeof(vec3), 1, out);
	fclose(out);
}

bool Lights::Read(const char *filename) {
	FILE *in = fopen(filename, "rb");
	if (!in) {
		printf("can't open %s\n", filename);
		return false;
	}
	int nlights;
	if (fread(&nlights, sizeof(int), 1, in) != 1)
		return false;
	points->resize(nlights);
	for (int i = 0; i < nlights; i++)
		if (fread(points->data()+i, sizeof(vec3), 1, in) != 1)
			return false;
	fclose(in);
	return true;
}
