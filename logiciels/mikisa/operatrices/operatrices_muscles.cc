/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "operatrices_muscles.hh"

#include <fstream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#include <Eigen/Eigen>
#pragma GCC diagnostic pop

#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/flux_chaine.hh"

#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "normaux.hh"

/**
 * Simulation de muscle
 * Code pris de https://github.com/harshavardhankode/MuscleSimulation
 *
 * Basé sur les papiers :
 * http://physbam.stanford.edu/~fedkiw/papers/stanford2004-06.pdf
 * https://www.math.ucla.edu/~jteran/papers/TSNF03.pdf
 * https://www.math.ucla.edu/~jteran/papers/ITF04.pdf
 */

/* ************************************************************************** */

class SimMesh;

class Joint {
public:
	int num_muscles{};
	float angle{},l1{},l2{}; // angle in radians, length in metres
	float weight{};

	dls::tableau<SimMesh*> muscleList{};
	dls::tableau<float> muscleOrigins{};	//0-1
	dls::tableau<float> muscleInsertions{};
	dls::tableau<float> muscleActivations{}; //0-1
	dls::tableau<float> muscleMaxForces{};
	dls::tableau<float> muscleLengths{};

	//If dummies are to be added
	dls::tableau<float> dummy_muscle_moments{};
	dls::tableau<float> dummy_muscle_max_force{};

	dls::tableau<dls::math::vec3f> muscleScales{};
	dls::tableau< dls::math::vec4f > render_buffer{};

	Joint(float,float,float);
	void AddMuscle(SimMesh* , float, float, float, const dls::math::vec3f &);

	void IncAngleDegrees(float angle);
	void IncAngle(float angle);
	void IncWeight (float wt);

	void Equilibriate();
	void Render();

	void CalculateActivations();

	//void Step();
};

struct tet_t{
	dls::math::vec4i vertices{};

	Eigen::Matrix3f F{}; // Deformation gradient

	Eigen::Matrix3f P{}; //Piola kirchoff stress

	Eigen::Vector3f g[4]; // Stress -> Force coefficients for each node
};

class SimMesh {
public:
	int num_vertices{};
	int num_faces{};
	int num_tets{};

	dls::tableau< dls::math::vec4f > vertices{};
	dls::tableau< dls::math::vec4f > curr_vertices{};
	dls::tableau< dls::math::vec4f > final_vertices{};
	dls::tableau< float > vertex_stress{};

	dls::tableau< dls::math::vec3f > faces{};
	dls::tableau< dls::math::vec4i > tets{};

	dls::tableau< Eigen::Vector3f > forces{};

	dls::tableau< tet_t* > sim_tets{};

	dls::tableau< dls::math::vec4f > render_buffer{};

	int origin_vertex{};
	int insertion_vertex{};

	float act{};
	float l{};
	float Tmax{};
	float l_init{}; //activation,rest length, max active stress;

	float avg_move{};
	float max_move{};

	int iteration{};

	SimMesh();

	~SimMesh()
	{
		for (auto tet : sim_tets) {
			memoire::deloge("tet_t", tet);
		}
	}

	void ReadTetgenMesh(dls::chaine mesh_name);

	void Render(bool show_stresses = false);

	void RenderWireframe();

	void RenderSlice();

	void CalculateDeformationGradients();

	void CalculateStresses();

	void ComputeForces();

	void TimeStep(float dT);

	void MoveInsertion(const dls::math::vec4f &t);

	void Equilibriate(float threshold, float dT);

	void SetAct(float a);
	void SetLen(float a_l);
	void SetTmax(float a_Tmax);

};

Joint::Joint(float a, float a_l1, float a_l2)
{
	num_muscles=0;
	angle = dls::math::degrees_vers_radians(a);
	l1 = a_l1;
	l2 = a_l2;
	weight = 100;
}

void Joint::AddMuscle(SimMesh* muscle , float origin, float insertion, float maxForce, dls::math::vec3f const &scale)
{
	INUTILISE(scale);

	muscleList.pousse(muscle);
	muscleOrigins.pousse(origin);
	muscleInsertions.pousse(insertion);
	muscleMaxForces.pousse(maxForce);
	muscleActivations.pousse(0);

	auto k1 = l1*(1-origin);
	auto k2 = l2*(insertion);
	auto n_scale = (std::sqrt(std::pow(k1, 2.0f) + std::pow(k2, 2.0f)-2.0f * k1 * k2 * std::cos(angle)))/2.0f;

	muscleLengths.pousse(n_scale*2.0f);
	muscleScales.pousse(dls::math::vec3f(n_scale,n_scale,n_scale));
	num_muscles++;
}

void Joint::IncAngleDegrees(float a_angle)
{
	angle += a_angle * constantes<float>::POIDS_DEG_RAD;
}

void Joint::IncAngle(float a_angle)
{
	angle += a_angle;
}

void Joint::IncWeight(float wt)
{
	weight += wt;
}

void Joint::CalculateActivations()
{
	dls::tableau<float> moment_arms;
	float temp_moment,ms_moment_arm,net_moment_arm;
	float k1,k2;
	ms_moment_arm=0;

	for (auto i=0; i < num_muscles; i++) {
		k1 = l1*(1-muscleOrigins[i]);
		k2 = l2*(muscleInsertions[i]);
		temp_moment = k1 * k2 * std::sin(angle) / (std::sqrt(std::pow(k1,2.0f) + std::pow(k2,2.0f)-2.0f*k1*k2*std::cos(angle) ) );
		//cout<<"temp_moment: "<<temp_moment<<endl;
		moment_arms.pousse(temp_moment*muscleMaxForces[i]);
		ms_moment_arm += std::pow(temp_moment*muscleMaxForces[i],2.0f);
	}

	//rms_moment_arm = sqrt(rms_moment);

	net_moment_arm = weight * l2 * std::sin(angle);

	//cout<<"nma: "<<net_moment_arm<<endl;
	//cout<<"msma: "<<ms_moment_arm<<endl;
	//std::cout<<glm::degrees(angle)<<" "<<weight<<" ";
	for (auto i=0; i < num_muscles; i++) {
		muscleActivations[i] = (moment_arms[i]*net_moment_arm)/ms_moment_arm;
		//cout<<"Muscle "<<i<<" activation: "<<muscleActivations[i]<<"; ";

		if (muscleActivations[i] < 0){
			muscleActivations[i] = 0;
		}

		if (muscleActivations[i] > 1){
			muscleActivations[i] = 1;
		}

		//	std::cout<< muscleActivations[i] << " ";
		//cout<<"Muscle "<<i<<" activation after truncating: "<<muscleActivations[i]<<"; ";
	}

	//std::cout<<std::endl;
}

void Joint::Equilibriate()
{
	CalculateActivations();

	for (auto i=0; i < num_muscles; i++) {
		auto k1 = l1 * (1.0f - muscleOrigins[i]);
		auto k2 = l2 * (muscleInsertions[i]);
		auto new_length = (std::sqrt(std::pow(k1,2.0f)+std::pow(k2,2.0f)-2.0f*k1*k2*std::cos(angle)));
		auto diff = new_length- muscleLengths[i];
		muscleLengths[i] = new_length;
		muscleList[i]->MoveInsertion(dls::math::vec4f(diff / muscleScales[i].x, 0.0f, 0.0f, 0.0f));
		muscleList[i]->SetAct(muscleActivations[i]);
		muscleList[i]->Equilibriate(1, 0.001f);
	}
}

void Joint::Render()
{
	// Add each vertex of faces in the order into the render buffer (TODR)
	render_buffer.efface();

//	render_buffer.pousse(dls::math::vec4f(0.0,0.0,0.0,1.0));
//	render_buffer.pousse(dls::math::vec4f(0.0,l1,0.0,1.0));
//	render_buffer.pousse(dls::math::vec4f(0.0,0.0,0.0,1.0));
//	dls::math::mat4x4f rotation = dls::math::rotation(dls::math::mat4x4f(1.0f), angle, dls::math::vec3f(0.0f,0.0f,-1.0f));
//	render_buffer.pousse(rotation*dls::math::vec4f(0.0,l2,0.0,1.0));
	/*for ( int i=0; i< faces.taille() ; i++){
		render_buffer.pousse(curr_vertices[faces[i].x]);
		render_buffer.pousse(curr_vertices[faces[i].y]);
		render_buffer.pousse(curr_vertices[faces[i].z]);
		//cout<<faces[i].x<<","<<faces[i].y<<","<<faces[i].z<<endl;
	}*/

	//setting up colour

//	render_buffer.pousse(dls::math::vec4f(1.0,1.0,1.0,1.0));
//	render_buffer.pousse(dls::math::vec4f(1.0,1.0,1.0,1.0));
//	render_buffer.pousse(dls::math::vec4f(1.0,1.0,1.0,1.0));
//	render_buffer.pousse(dls::math::vec4f(1.0,1.0,1.0,1.0));
	/*for ( int i=0; i< faces.taille() ; i++){
		render_buffer.pousse(red_blue(forces[faces[i].x].norm(),500,2000));
		render_buffer.pousse(red_blue(forces[faces[i].y].norm(),500,2000));
		render_buffer.pousse(red_blue(forces[faces[i].z].norm(),500,2000));
	}*/

	// setup the vbo and vao;
	//cout<<render_buffer.taille()<<endl;
	//bind them
//	glBindVertexArray (vao);
//	glBindBuffer (GL_ARRAY_BUFFER, vbo);

//	glBufferData (GL_ARRAY_BUFFER, render_buffer.taille() * sizeof(dls::math::vec4f), &render_buffer[0], GL_STATIC_DRAW);
//	//cout<< render_buffer.taille() * sizeof(dls::math::vec4f)<<endl;
//	//setup the vertex array as per the shader
//	glEnableVertexAttribArray( vPosition );
//	glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

//	size_t offset = (1.0/2)*render_buffer.taille()* sizeof(dls::math::vec4f);

//	glEnableVertexAttribArray( vColor );
//	glVertexAttribPointer( vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(offset));


//	dls::math::mat4x4f* ms_mult = multiply_stack(matrixStack);

//	glUniformMatrix4fv(uModelViewMatrix, 1, GL_FALSE, glm::value_ptr(*ms_mult));




//	glBindVertexArray (vao);

	/*if (render_wireframe){
		for (int i = 0; i < num_faces*3; i += 3)
			glDrawArrays(GL_LINE_LOOP, i, 3);
	}
	else{
		glDrawArrays(GL_TRIANGLES, 0, num_faces*3);
	}*/

//	glDrawArrays(GL_LINES, 0, 4);

	auto pi = constantes<float>::PI;
	for (auto i = 0l; i < muscleList.taille(); i++){
		auto k1 = l1 * (1.0f - muscleOrigins[i]);
		auto k2 = l2 * (muscleInsertions[i]);
		auto d = (std::sqrt(std::pow(k1, 2.0f) + std::pow(k2,2.0f)-2.0f*k1*k2*std::cos(angle)));

		auto muscle_angle = pi * 0.5f - std::asin(  k2*std::sin(angle)/(d));

		if (std::pow(k1, 2.0f) + std::pow(d, 2.0f) < std::pow(k2, 2.0f)) {
			muscle_angle = -muscle_angle;
		}
		//cout<<muscle_angle<<endl;

//		matrixStack.pousse(glm::translate(dls::math::mat4x4f(1.0f), dls::math::vec3f(0.0f,l1*(1-muscleOrigins[i]),0.0f)));

//		matrixStack.pousse(glm::rotate(dls::math::mat4x4f(1.0f),muscle_angle, dls::math::vec3f(0.0f,0.0f,-1.0f)) );

//		matrixStack.pousse(glm::scale(dls::math::mat4x4f(1.0f),dls::math::vec3f(muscleScales[i])));

		muscleList[i]->Render();

//		matrixStack.pop_back();
//		matrixStack.pop_back();
//		matrixStack.pop_back();
	}
}

dls::math::mat4x4f multiply_stack(dls::tableau<dls::math::mat4x4f> matStack)
{
	auto mult = dls::math::mat4x4f(1.0f);

	for (auto i=0l; i < matStack.taille();i++){
		mult *= matStack[i];
	}

	return mult;
}

static auto red_blue(float value, float lo, float hi)
{
	float f;
	//cout<<f<<endl;
	if (value<lo){
		f = 0.0f;
	}
	else if (value>hi){
		f = 1.0f;
	}
	else{
		f = (value-lo)/(hi - lo);
	}
	//cout<<f<<endl;
	return dls::math::vec3f(f, 1.0f - f, 0.0f);
	//return dls::math::vec4f(1.0,1.0,0.0,1.0);
}

SimMesh::SimMesh()
{
	iteration = 0;
	act = 1;
	origin_vertex = 0;
	insertion_vertex = 17;
	Tmax =80000;
	l=l_init=1.0;
}

void SimMesh::ReadTetgenMesh(dls::chaine mesh_name){

	//Read vertices
	std::string line;
	dls::flux_chaine temp_ss;

	std::ifstream node_file((mesh_name+".node").c_str());

	if (node_file.is_open())
	{
		while ( getline (node_file,line) )
		{
			if (line[line.find_first_not_of(" ")] == '#' )
				continue;
			else{
				temp_ss.chn(line.c_str());
				temp_ss >> num_vertices;
				break;
			}
		}
		while ( getline (node_file,line) ){
			if (line[line.find_first_not_of(" ")] == '#' )
				continue;
			else{
				temp_ss.chn(line.c_str());
				int counter;
				dls::math::vec4f vertex;
				temp_ss >> counter >> vertex.x >> vertex.y >> vertex.z;
				vertex.w = 1;
				vertices.pousse(vertex);
				vertex_stress.pousse(0);
			}

		}

	}
	else{
		std::cout<<"could not open mesh node file\n";
	}

	//Read faces
	std::ifstream face_file((mesh_name + ".face").c_str());

	if (face_file.is_open())
	{
		while ( getline (face_file,line) )
		{
			if (line[line.find_first_not_of(" ")] == '#' )
				continue;
			else{
				temp_ss.chn(line.c_str());
				temp_ss >> num_faces;
				//cout<<num_faces<<endl;
				break;
			}
		}
		while ( getline (face_file,line) ){
			if (line[line.find_first_not_of(" ")] == '#' )
				continue;
			else{
				line +=" ";
				temp_ss.chn(line.c_str());
				int counter;
				dls::math::vec3f face;
				temp_ss >> counter >> face.x >> face.y >> face.z;
				// indices in vertices array start from 0
				face.x--;face.y--;face.z--;
				faces.pousse(face);
			}

		}
	}


	//Read Tetrahedra
	std::ifstream tets_file((mesh_name + ".ele").c_str());

	if (tets_file.is_open())
	{
		while ( getline (tets_file,line) )
		{
			if (line[line.find_first_not_of(" ")] == '#' )
				continue;
			else{
				temp_ss.chn(line.c_str());
				temp_ss >> num_tets;
				//cout<<num_tets<<endl;
				break;
			}
		}
		while ( getline (tets_file,line) ){
			if (line[line.find_first_not_of(" ")] == '#' )
				continue;
			else{
				line +=" ";
				temp_ss.chn(line.c_str());
				int counter;
				dls::math::vec4i tet;
				temp_ss >> counter >> tet.x >> tet.y >> tet.z >> tet.w;
				// indices in vertices array start from 0
				tet.x--;tet.y--;tet.z--;tet.w--;
				tets.pousse(tet);
			}
		}
	}

	// initialize sim tets
	for (auto i = 0l; i < tets.taille(); i++){
		auto temp = memoire::loge<tet_t>("tet_t");
		temp->vertices = tets[i];
		sim_tets.pousse(temp);

		// initialize the stress-force coefficients
		Eigen::Vector3f v0;
		v0 << vertices[temp->vertices[0]].x, vertices[temp->vertices[0]].y, vertices[temp->vertices[0]].z;
		Eigen::Vector3f v1;
		v1 << vertices[temp->vertices[1]].x ,vertices[temp->vertices[1]].y ,vertices[temp->vertices[1]].z;
		Eigen::Vector3f v2;
		v2 << vertices[temp->vertices[2]].x ,vertices[temp->vertices[2]].y ,vertices[temp->vertices[2]].z;
		Eigen::Vector3f v3;
		v3 << vertices[temp->vertices[3]].x ,vertices[temp->vertices[3]].y ,vertices[temp->vertices[3]].z;

		temp->g[0] = -(1.0/6.0)*((v2-v0).cross(v1-v0) + (v1-v0).cross(v3-v0) + (v3-v0).cross(v2-v0));
		temp->g[1] = -(1.0/6.0)*((v3-v1).cross(v0-v1) + (v0-v1).cross(v2-v1) + (v2-v1).cross(v3-v1));
		temp->g[2] = -(1.0/6.0)*((v3-v2).cross(v1-v2) + (v1-v2).cross(v0-v2) + (v0-v2).cross(v3-v2));
		temp->g[3] = -(1.0/6.0)*((v1-v3).cross(v2-v3) + (v2-v3).cross(v0-v3) + (v0-v3).cross(v1-v3));
	}

	curr_vertices = final_vertices = vertices; // No deformation for now
}

void SimMesh::Equilibriate(float threshold, float dT){



	do{
		CalculateDeformationGradients();
		CalculateStresses();
		ComputeForces();
		TimeStep(dT);
		iteration++;
	}while(max_move > threshold);

	//cout<<"Equilibriated at iteration " <<iteration<<endl;


}

void SimMesh::Render(bool show_stresses)
{
	//cout<<vertices.taille()<<endl;
	// Add each vertex of faces in the order into the render buffer
	render_buffer.efface();

	for (auto f : faces) {
		render_buffer.pousse(curr_vertices[static_cast<long>(f.x)]);
		render_buffer.pousse(curr_vertices[static_cast<long>(f.y)]);
		render_buffer.pousse(curr_vertices[static_cast<long>(f.z)]);
	}

	//setting up colour
	if (show_stresses == false) {
//		for (auto f : faces) {
//			render_buffer.pousse(red_blue(forces[static_cast<long>(f.x)].norm(),500,2000));
//			render_buffer.pousse(red_blue(forces[static_cast<long>(f.y)].norm(),500,2000));
//			render_buffer.pousse(red_blue(forces[static_cast<long>(f.z)].norm(),500,2000));
//		}
	}
	else {
//		for (auto f : faces) {
//			render_buffer.pousse(red_blue(vertex_stress[static_cast<long>(f.x)],100000,500000));
//			render_buffer.pousse(red_blue(vertex_stress[static_cast<long>(f.y)],100000,500000));
//			render_buffer.pousse(red_blue(vertex_stress[static_cast<long>(f.z)],100000,500000));
//		}
	}
}

void SimMesh::CalculateDeformationGradients(){

	for (auto &tet : sim_tets) {
		Eigen::Matrix3f Dm;
		Eigen::Matrix3f Ds;
		Eigen::Matrix3f TempF;

		dls::math::vec4f edge1 = curr_vertices[tet->vertices[1]] -curr_vertices[tet->vertices[0]];
		dls::math::vec4f edge2 = curr_vertices[tet->vertices[3]] -curr_vertices[tet->vertices[0]];
		dls::math::vec4f edge3 = curr_vertices[tet->vertices[2]] -curr_vertices[tet->vertices[0]];

		Dm <<edge1.x,edge2.x,edge3.x,edge1.y,edge2.y,edge3.y,edge1.z,edge2.z,edge3.z;

		edge1 = vertices[tet->vertices[1]] -vertices[tet->vertices[0]];
		edge2 =	vertices[tet->vertices[3]] -vertices[tet->vertices[0]];
		edge3 = vertices[tet->vertices[2]] -vertices[tet->vertices[0]];

		Ds <<edge1.x,edge2.x,edge3.x,edge1.y,edge2.y,edge3.y,edge1.z,edge2.z,edge3.z;


		//Checking for inversion
		//tet->F = Ds*Dm.inverse();
		TempF = Ds*(Dm.inverse());
		// if (TempF.determinant() < 0){ //= 0 case?
		// 	Eigen::JacobiSVD<Eigen::Matrix3f> svd(TempF, Eigen::ComputeFullU | Eigen::ComputeFullV);

		// 	Eigen::Vector3f Sv = svd.singularValues();
		// 	Eigen::Matrix3f U = svd.matrixU();
		// 	Eigen::Matrix3f V = svd.matrixV();
		// 	cout << "Its singular values are:" << endl << Sv << endl;
		// 	cout << "U matrix:" << endl << U << endl << U.determinant() << endl;
		// 	cout << "V matrix:" << endl << V << endl << V.determinant() << endl;

		// 	int minC;
		// 	Sv.minCoeff(&minC);
		// 	Sv[minC]= - Sv[minC];
		// 	cout << "Its new values are:" << endl << Sv << endl;
		// 	Eigen::Matrix3f S = Sv.asDiagonal();
		// 	cout << "S matrix after invert:" << endl << S << endl;

		// 	TempF = U*S*V.transpose();
		// }
		tet->F = TempF;
		//cout<<tet->F<<endl;
	}
}

void SimMesh::CalculateStresses(){

	Eigen::Vector3f fm,fmR ; // fiber direction
	fm<<1,0,0; // initialised to x direction, teporary
	fmR<<1,0,0;
	// À FAIRE later add fiber direction in each tet

	Eigen::Matrix3f F,F_orig,P,P_r,P_r1,P_r2,P_r3,P_r4,P_r123;
	Eigen::Matrix3f C; // Cauchy stress
	Eigen::Matrix3f I1;
	float Jc,Jcc,w1,w2,w12,i1,lambda,p,pf;
	float mat_c1 = 30000; // (muscle)
	//float mat_c1 = 60000 // (tendon),
	float mat_c2 = 10000; // (muscle and tendon),
	float K = 60000; // (muscle)
	//float K = 80000;// (tendon)
	float T = Tmax*act*(std::max(1.0f-(std::abs(l-l_init)*0.5f),0.0f));//80000;

	// Main loop for each tetrahedron
	for (auto i = 0l; i < sim_tets.taille();i++) {
		F_orig = sim_tets[i]->F;

		if ( true /*F_orig.determinant() < 0*/){
			Eigen::JacobiSVD<Eigen::Matrix3f> svd(F_orig, Eigen::ComputeFullU | Eigen::ComputeFullV);

			Eigen::Vector3f Sv = svd.singularValues();
			Eigen::Matrix3f U = svd.matrixU();
			Eigen::Matrix3f V = svd.matrixV();
			// cout << "Its singular values are:" << endl << Sv << endl;
			// cout << "U matrix:" << endl << U << endl << U.determinant() << endl;
			// cout << "V matrix:" << endl << V << endl << V.determinant() << endl;
			for (int k=0;k<3;k++){
				if (Sv(k)<0)
					Sv(k) = -Sv(k);
			}
			F = Sv.asDiagonal();


			//bool Vflag = false; // Represents if V has been negated i.e one column inverted
			// Making V a rotation
			if (V.determinant() < 0){
				V.col(0) = -V.col(0);
			//	Vflag = true;
				//cout << "V matrix inverted to make it rotation:" << endl << V << endl << V.determinant() << endl;
			}

			// Using the method given in the paper to find U

			U = F_orig* V * F.inverse();
			// Inverting corresponding column in U and corresponding column in F, but maintaining sign
			if (U.determinant() < 0 ){

				int minC;
				Sv.minCoeff(&minC);
				Sv[minC]= - Sv[minC];

				//cout << "F matrix in the rotated space, after invering the least element:" << endl << F << endl;

				U.col(minC) = -U.col(minC);
				//cout << "U matrix inverted to make it rotation:" << endl << U << endl << U.determinant() << endl;

			}
			//cout<<"Dets of F and U:" << F.determinant() << " and " <<U.determinant()<<endl;

			// if (U.determinant() < 0 && F.determinant() > 0){
			// 	U.col(0) = -U.col(0);
			// }
			//}

			F = Sv.asDiagonal();

			auto threshold = 0.2f;
			Jc = F.determinant();

			if (Jc < 0){
				std::cout<<"Invert at iteration "<<iteration<<", Tet:"<<i<<std::endl;
				//cout << "F matrix in the rotated space" << endl << F << endl;
				// Thresholding the deformation gradient to handle inversion
				for (int j=0;j<3;j++){
					if (F(j,j) < threshold){
						F(j,j) = threshold;
					}
				}

				std::cout << "F matrix after Thresholding" << std::endl << F << std::endl;
			}

			Jc = F.determinant();
			Jcc = std::pow(Jc, 2.0f);

			/*if (Jc < 0){
				Jc = -(1.0/pow(-Jc,(1.0/3)));
			}
			else{
				Jc = (1.0/pow(Jc,(1.0/3)));
			}*/

			Jcc = (1.0f / std::pow(Jcc, (1.0f / 3.0f)));
			Jcc = 1.0f / Jcc;

			fmR = V.transpose()*fm;
			C = F.transpose() * F;
			I1 = Jcc*C;
			i1 = I1.trace();
			lambda = std::sqrt(fmR.transpose() * C * fmR);

			w1 = 4.0f * Jcc * mat_c1;
			w2 = 4.0f * std::pow(Jcc,2.0f) * mat_c2;
			w12 = w1 + i1*w2;

			p = K * std::log(Jcc); // Jc / Jcc
			pf = (1.0f / 3.0f) * (w12 * C.trace() - w2 * ((C * C).trace()) + T * std::pow(lambda, 2.0f));

			//cout<<"p:"<<p<<" pf:"<<pf<<endl;

			if (F.determinant() < 0){
				std::cout<<"w1: "<<w1<<std::endl;
			}

			//cout<<(1.0/3.0)*w12*C.trace() - w2*((C*C).trace())<<" "<<T*pow(lambda,2)<<" \n";
			P_r1 = w12*(F) ;
			P_r2 = - w2*(F*F*F);
			P_r3 = (p - pf)*(F.inverse());
			P_r4 = 4.0f*Jcc*T*(F*fmR)*fmR.transpose();

			auto Plo = -99999.0f,Phi = 99999.0f;

			P_r123 = P_r1+P_r2+P_r3;

			for (int j=0;j<3;j++){
				if (P_r123(j,j) < Plo){
					P_r123(j,j) = Plo;
				}
				if (P_r123(j,j) > Phi){
					P_r123(j,j) = Phi;
				}
			}

			if (Jc < 0){
				for (int j=0;j<3;j++){
					P_r4(j,0) = 0;
					P_r4(j,1) = 0;
					P_r4(j,2) = 0;
				}
			}

			P_r = P_r123+ P_r4;

			/*if (vertices[i].x < 0.2 || vertices[i].x >1.8){

			}
			else{
				P_r = P_r123+ P_r4;
			}*/


			for (int j=0;j<3;j++){
				for (int k=0;k<3;k++){
					if (std::isnan(P_r1(k,j))){
						std::cout<<"r1 (" << k<<","<<j<<") is NaN at iteration "<<iteration<<", Tet:"<<i<<std::endl;

						//exit(0);
					}
					if (std::isnan(P_r2(k,j))){
						std::cout<<"r2 (" << k<<","<<j<<") is NaN at iteration "<<iteration<<", Tet:"<<i<<std::endl;

						//exit(0);
					}
					if (std::isnan(P_r3(k,j))){
						std::cout<<"r3 (" << k<<","<<j<<") is NaN at iteration "<<iteration<<", Tet:"<<i<<std::endl;

						//exit(0);
					}
					if (std::isnan(P_r4(k,j))){
						std::cout<<"r4 (" << k<<","<<j<<") is NaN at iteration "<<iteration<<", Tet:"<<i<<std::endl;

						//exit(0);
					}
				}
			}

			P = U * P_r * V.transpose();

			//std::cout << "P in rotated space and p1, p2 " << endl << P_r <<std::endl<<P_r1 <<std::endl<<P_r2<< std::endl;

			sim_tets[i]->P = P;

			//std::cout<<P<<std::endl;*/
		}
		else {
			F = F_orig;
			Jc = F.determinant();

			if (Jc < 0){
				std::cout<<"Invert"<<std::endl;
			}

			Jc = 1.0f / std::pow(Jc, (1.0f / 3.0f));
			Jcc = std::pow(Jc, 2.0f);
			C = F.transpose() * F;
			I1 = Jcc*C;
			i1 = I1.trace();
			lambda = std::sqrt(fm.transpose() * C * fm);

			w1 = 4.0f * Jcc * mat_c1;
			w2 = 4.0f * std::pow(Jcc, 2.0f) * mat_c2;
			w12 = w1 + i1*w2;

			p = K * std::log(Jc); // Jc / Jcc
			pf = (1.0f/3.0f)*(w12*C.trace() - w2*((C*C).trace()) + T*std::pow(lambda,2.0f));

			P = w12*F - w2*(F*F*F) + (p - pf)*(F.inverse()) + 4*Jcc*T*(F*fm)*fm.transpose();
			sim_tets[i]->P = P;
		}
	}
}

void SimMesh::ComputeForces()
{
	forces.efface();
	for (auto i = 0l; i < vertices.taille(); i++){
		Eigen::Vector3f a;
		a<<0,0,0;
		forces.pousse(a);
		vertex_stress[i]=0;
	}

	Eigen::Matrix3f curr_P;
	for (auto curr_tet : sim_tets){
		curr_P = curr_tet->P;

		for (auto j = 0ul; j < 4; j++){
			auto curr_vertex = curr_tet->vertices[j];
			forces[curr_vertex]-=(curr_P)*(curr_tet->g[j]);
			vertex_stress[curr_vertex] += 0.25f * (curr_P(0,0) + curr_P(1,1) + curr_P(2,2));
			//cout<<"iteration "<<iteration<<"vertex stress"<< i<<"th vertex:"<<vertex_stress[i]<<endl;
			//forces=
		}
	}

	//for (int i=0;i<num_vertices;i++){
		//std::cout<<forces[i].norm()<<endl;
	//}
}

void SimMesh::MoveInsertion(dls::math::vec4f const &t)
{
	for (auto i = 0l; i < vertices.taille(); ++i){
		if (vertices[i].x > 1.8f){ //condition for insertion vertices
			curr_vertices[i] += t;
		}
	}

	l+=t.x;
}

void SimMesh::TimeStep(float dT)
{
	// 1 and 18 are clamped = 0 & 17 in the array
	dls::math::vec4f curr_force;
	avg_move = 0;
	max_move=0;
	float move;
	for (auto i = 0l; i < vertices.taille(); ++i){
		//if (i == 0 || i==17){
		if (vertices[i].x < 0.2f || vertices[i].x >1.8f){
			// if (iteration==0){
			// 	curr_vertices[i].x +=0.80;

			// }
			continue;
		}
		curr_force = dls::math::vec4f(forces[i][0],forces[i][1],forces[i][2],0.0f);
		curr_vertices[i] = curr_vertices[i] + curr_force*dT*dT*1.0f; //Later implement the mass here
		move = longueur(curr_force * dT * dT * 1.0f);
		avg_move+=move;
		if (max_move < move)
			max_move = move;
		// if (i==0 && iteration == 0){
		// 	curr_vertices[i].x +=0.20;
		// 	//std::cout<<glm::to_string(curr_force*dT*dT*1.0f)<<endl;
		// }

	}

	avg_move=avg_move/static_cast<float>(num_vertices);
}

void SimMesh::SetAct(float a)
{
	act = a;
}

void SimMesh::SetLen(float a_l)
{
	l = a_l;
}

void SimMesh::SetTmax(float a_Tmax)
{
	Tmax = a_Tmax;
}

/* ************************************************************************** */

class OpSimMuscles : public OperatriceCorps {
	SimMesh *mesh1 = nullptr;
	SimMesh *mesh2 = nullptr;
	Joint *joint1 = nullptr;

public:
	static constexpr auto NOM = "Simulation Muscles";
	static constexpr auto AIDE = "";

	explicit OpSimMuscles(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	~OpSimMuscles() override
	{
		memoire::deloge("SimMesh", mesh1);
		memoire::deloge("SimMesh", mesh2);
		memoire::deloge("Joint", joint1);
	}

	OpSimMuscles(OpSimMuscles const &) = default;
	OpSimMuscles &operator=(OpSimMuscles const &) = default;

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);

		m_corps.reinitialise();

		if (contexte.temps_courant == 1) {
			memoire::deloge("SimMesh", mesh1);
			memoire::deloge("SimMesh", mesh2);
			memoire::deloge("Joint", joint1);

			mesh1 = memoire::loge<SimMesh>("SimMesh");
			mesh2 = memoire::loge<SimMesh>("SimMesh");
			mesh1->ReadTetgenMesh("/opt/bin/mikisa/test/muscle1.2");
			mesh2->ReadTetgenMesh("/opt/bin/mikisa/test/muscle1.2");

			joint1 = memoire::loge<Joint>("Joint", 90.0f, 1.0f, 1.0f);
			joint1->AddMuscle(mesh1, 0.1f, 0.2f, 1000, dls::math::vec3f(0.4f));
			joint1->AddMuscle(mesh2, 0.8f, 0.8f, 500, dls::math::vec3f(0.4f));

			convertie_geometrie();
			return EXECUTION_REUSSIE;
		}

		joint1->Equilibriate();
		convertie_geometrie();

		return EXECUTION_REUSSIE;
	}

	void convertie_geometrie()
	{
		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);

		/* pour le joint */
		m_corps.ajoute_point(0.0f, 0.0f, 0.0f);
		m_corps.ajoute_point(0.0f, joint1->l1, 0.0f);

		m_corps.ajoute_point(0.0f, 0.0f, 0.0f);

		auto rotation = dls::math::rotation(dls::math::mat4x4f(1.0f), joint1->angle, dls::math::vec3f(0.0f,0.0f,-1.0f));
		auto p = rotation * dls::math::vec4f(0.0f, joint1->l2, 0.0f, 1.0f);
		m_corps.ajoute_point(p.x, p.y, p.z);

		attr_C->pousse(dls::math::vec3f(1.0f));
		attr_C->pousse(dls::math::vec3f(1.0f));
		attr_C->pousse(dls::math::vec3f(1.0f));
		attr_C->pousse(dls::math::vec3f(1.0f));

		auto poly = Polygone::construit(&m_corps, type_polygone::OUVERT, 2);
		poly->ajoute_sommet(0);
		poly->ajoute_sommet(1);

		poly = Polygone::construit(&m_corps, type_polygone::OUVERT, 2);
		poly->ajoute_sommet(2);
		poly->ajoute_sommet(3);

		/* pour les muscles */
		convertie_geometrie_muscle(mesh1, 0, true, attr_C);
		convertie_geometrie_muscle(mesh2, 1, true, attr_C);

		calcul_normaux(m_corps, true, false);
	}

	void convertie_geometrie_muscle(SimMesh *mesh, long i, bool show_stresses, Attribut *attr_C)
	{
		auto decalage = m_corps.points()->taille();

		auto pi = constantes<float>::PI;

		auto k1 = joint1->l1 * (1.0f - joint1->muscleOrigins[i]);
		auto k2 = joint1->l2 * (joint1->muscleInsertions[i]);
		auto d = (std::sqrt(std::pow(k1, 2.0f) + std::pow(k2,2.0f)-2.0f*k1*k2*std::cos(joint1->angle)));

		auto muscle_angle = pi * 0.5f - std::asin(  k2*std::sin(joint1->angle)/(d));

		if (std::pow(k1, 2.0f) + std::pow(d, 2.0f) < std::pow(k2, 2.0f)) {
			muscle_angle = -muscle_angle;
		}

		auto mat = dls::math::mat4x4f(1.0f);
		mat *= dls::math::translation(dls::math::mat4x4f(1.0f), dls::math::vec3f(0.0f, joint1->l1*(1.0f - joint1->muscleOrigins[i]), 0.0f));
		mat *= dls::math::rotation(dls::math::mat4x4f(1.0f), muscle_angle, dls::math::vec3f(0.0f,0.0f,-1.0f));
		mat *= dls::math::dimension(dls::math::mat4x4f(1.0f), joint1->muscleScales[i]);

		for (auto f : mesh->faces) {
			auto const &v0 = mat * mesh->curr_vertices[static_cast<long>(f.x)];// + mat[3];
			auto const &v1 = mat * mesh->curr_vertices[static_cast<long>(f.y)];// + mat[3];
			auto const &v2 = mat * mesh->curr_vertices[static_cast<long>(f.z)];// + mat[3];

			m_corps.ajoute_point(v0.x, v0.y, v0.z);
			m_corps.ajoute_point(v1.x, v1.y, v1.z);
			m_corps.ajoute_point(v2.x, v2.y, v2.z);

			auto poly = Polygone::construit(&m_corps, type_polygone::FERME, 3);
			poly->ajoute_sommet(decalage++);
			poly->ajoute_sommet(decalage++);
			poly->ajoute_sommet(decalage++);
		}

		if (show_stresses == false) {
			for (auto f : mesh->faces) {
				attr_C->pousse(red_blue(mesh->forces[static_cast<long>(f.x)].norm(), 500.0f, 2000.0f));
				attr_C->pousse(red_blue(mesh->forces[static_cast<long>(f.y)].norm(), 500.0f, 2000.0f));
				attr_C->pousse(red_blue(mesh->forces[static_cast<long>(f.z)].norm(), 500.0f, 2000.0f));
			}
		}
		else {
			for (auto f : mesh->faces) {
				attr_C->pousse(red_blue(mesh->vertex_stress[static_cast<long>(f.x)], 100000.0f, 500000.0f));
				attr_C->pousse(red_blue(mesh->vertex_stress[static_cast<long>(f.y)], 100000.0f, 500000.0f));
				attr_C->pousse(red_blue(mesh->vertex_stress[static_cast<long>(f.z)], 100000.0f, 500000.0f));
			}
		}
	}
};

/* ************************************************************************** */

void enregistre_operatrices_muscles(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OpSimMuscles>());
}
