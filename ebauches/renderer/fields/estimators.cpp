#include <glm/glm.hpp>

float signed_sphere(const glm::vec3 &pos, float radius)
{
	return glm::length(pos) - radius;
}

float signed_box(const glm::vec3 &pos, glm::vec3 b)
{
	glm::vec3 d = glm::abs(pos) - b;
	return glm::min(glm::max(d.x, glm::max(d.y, d.z)), 0.0f) + glm::length(glm::max(d, 0.0f));
}

float unsigned_box(const glm::vec3 &pos, glm::vec3 b)
{
	return glm::length(glm::max(glm::abs(pos) - b, 0.0f));
}

float unsigned_roundbox(const glm::vec3 &pos, glm::vec3 b, float r)
{
	return glm::length(glm::max(glm::abs(pos) - b, 0.0f)) - r;
}

float sdTorus(const glm::vec3 &pos, glm::vec2 t)
{
	glm::vec2 q = vec2(length(pos.xz) - t.x, pos.y);
	return glm::length(q) - t.y;
}

float sdCylinder(const glm::vec3 &pos, glm::vec3 c)
{
	return glm::length(p.xz-c.xy)-c.z;
}

float sdCone(const glm::vec3 &pos, glm::vec2 c)
{
	// c must be normalized
	float q = glm::length(glm::vec2(pos.x, pos.y));
	return glm::dot(c, glm::vec2(q, pos.z));
}

float sdPlane(const glm::vec3 &pos, glm::vec4 n)
{
	// n must be normalized
	return dot(pos, n.xyz) + n.w;
}

float sdHexPrism(const glm::vec3 &pos, glm::vec2 h)
{
	glm::vec3 q = glm::abs(pos);
	return glm::max(q.z - h.y, glm::max((q.x * 0.866025f + q.y * 0.5f), q.y) - h.x);
}

float sdTriPrism(const glm::vec3 &pos, glm::vec2 h )
{
	glm::vec3 q = abs(pos);
	return max(q.z-h.y,max(q.x*0.866025+pos.y*0.5,-pos.y)-h.x*0.5);
}

float sdCapsule(const glm::vec3 &pos, glm::vec3 a, glm::vec3 b, float r )
{
	glm::vec3 posa = pos - a, ba = b - a;
	float h = clamp( glm::dot(posa,ba)/glm::dot(ba,ba), 0.0, 1.0 );
	return glm::length( posa - ba*h ) - r;
}

float sdCappedCylinder(const glm::vec3 &pos, glm::vec2 h)
{
	vec2 d = glm::abs(glm::vec2(glm::length(p.xz),p.y)) - h;
	return glm::min(glm::max(d.x,d.y),0.0) + glm::length(glm::max(d,0.0));
}

float dot2(const glm::vec3 &v)
{
	return glm::dot(v, v);
}

float udTriangle(const glm::vec3 &pos, const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c)
{
	glm::vec3 ba = b - a; glm::vec3 posa = pos - a;
	glm::vec3 cb = c - b; glm::vec3 posb = pos - b;
	glm::vec3 ac = a - c; glm::vec3 posc = pos - c;
	glm::vec3 nor = glm::cross( ba, ac );

	return sqrt(
			(sign(dot(cross(ba,nor),posa)) +
			 sign(dot(cross(cb,nor),posb)) +
			 sign(dot(cross(ac,nor),posc))<2.0)
			?
			min( min(
					dot2(ba*clamp(dot(ba,pa)/dot2(ba),0.0,1.0)-pa),
					dot2(cb*clamp(dot(cb,pb)/dot2(cb),0.0,1.0)-pb) ),
				dot2(ac*clamp(dot(ac,pc)/dot2(ac),0.0,1.0)-pc) )
			:
			dot(nor,pa)*dot(nor,pa)/dot2(nor) );
}

float udQuad(const glm::vec3 &p,
             const glm::vec3 &a, const glm::vec3 &b,
             const glm::vec3 &c, const glm::vec3 &d)
{
	glm::vec3 ba = b - a;
	glm::vec3 pa = p - a;
	glm::vec3 cb = c - b;
	glm::vec3 pb = p - b;
	glm::vec3 dc = d - c;
	glm::vec3 pc = p - c;
	glm::vec3 ad = a - d;
	glm::vec3 pd = p - d;
	glm::vec3 nor = glm::cross(ba, ad);

	return sqrt(
			(sign(dot(cross(ba,nor),pa)) +
			 sign(dot(cross(cb,nor),pb)) +
			 sign(dot(cross(dc,nor),pc)) +
			 sign(dot(cross(ad,nor),pd))<3.0)
			?
			min( min( min(
						dot2(ba*clamp(dot(ba,pa)/dot2(ba),0.0,1.0)-pa),
						dot2(cb*clamp(dot(cb,pb)/dot2(cb),0.0,1.0)-pb) ),
					dot2(dc*clamp(dot(dc,pc)/dot2(dc),0.0,1.0)-pc) ),
				dot2(ad*clamp(dot(ad,pd)/dot2(ad),0.0,1.0)-pd) )
			:
			dot(nor,pa)*dot(nor,pa)/dot2(nor) );
}

float sdCappedCone(const glm::vec3 &pos, const glm::vec3 &c)
{
	glm::vec2 q = glm::vec2( glm::length(p.xz), p.y );
	glm::vec2 v = glm::vec2( c.z*c.y/c.x, -c.z );

	glm::vec2 w = v - q;

	glm::vec2 vv = glm::vec2( dot(v,v), v.x*v.x );
	glm::vec2 qv = glm::vec2( dot(v,w), v.x*w.x );

	vec2 d = max(qv,0.0)*qv/vv;

	return sqrt( dot(w,w) - max(d.x,d.y) )* sign(max(q.y*v.x-q.x*v.y,w.y));
}

float union_op(float d1, float d2)
{
	return glm::min(d1, d2);
}

float difference_op(float d1, float d2)
{
	return glm::max(-d1, d2);
}

float intersect_op(float d1, float d2)
{
	return glm::max(d1, d2);
}

float instantiate(const glm::vec3 &pos, const glm::vec3 &c)
{
	const glm::vec3 &q = glm::mod(pos, c) - 0.5f * c;
	return primitive(q);
}

glm::vec3 translate(const glm::vec3 &pos, glm::mat4 m)
{
	const glm::vec3 &q = glm::invert(m) * pos;
	return primitive(q);
}

float scale(const glm::vec3 &pos, float s)
{
	return primitive(pos / s) * s;
}

float displace(const glm::vec3 &pos)
{
	float d1 = primitive(pos);
	float d2 = displacement(pos);
	return d1+d2;
}

float blend(const glm::vec3 &pos)
{
	float d1 = primitiveA(pos);
	float d2 = primitiveB(pos);
	return smin(d1, d2);
}


