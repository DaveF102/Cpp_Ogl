#include "viewcontrols.h"

void ViewControls::SetViewX(float vwx)
{
	_viewX = vwx;
}

void ViewControls::SetViewY(float vwy)
{
	_viewY = vwy;
}

void ViewControls::SetPan(glm::vec3 pan)
{
	_pan = pan;
}

void ViewControls::SetZoom(float zm)
{
	_zoom = zm;
}