import bpy
import math

def formate_nombre(x):
    if (x >= 0.0):
        return ' {:.16f}'.format(x)

    return '{:.16f}'.format(x)

obj = bpy.data.objects['Cube']
mesh = obj.data

vertices = mesh.vertices
polys = mesh.polygons

chaine = "static glm::vec3 donnees_sommets[" + str(len(sommets)) + "] = {\n"

for vertice in vertices:
    chaine += "glm::vec3("
    chaine += formate_nombre(vertice.co[0]) + ", "
    chaine += formate_nombre(vertice.co[1]) + ", "
    chaine += formate_nombre(vertice.co[2]) + "),\n"

chaine += "};\n"
chaine += "\n"

chaine_index = ""
nombre_index = 0

for poly in polys:
    sommets_polygone = poly.sommets

    for i in range(len(sommets_polygone)):
        if i > 1:
            chaine_index += " " + str(sommets_polygone[i - 1]) + ","
            nombre_index += 1

        chaine_index += " " + str(sommets_polygone[i]) + ","
        nombre_index += 1

    chaine_index += "\n"

chaine += "static unsigned int donnees_index[" + str(nombre_index) + "] = {\n"
chaine += chaine_index
chaine += "};\n"

chaine += "std::vector<glm::vec3> sommets(std::begin(donnees_sommets), std::end(donnees_sommets));\n"
chaine += "std::vector<unsigned int> index(std::begin(donnees_index), std::end(donnees_index));\n"

print(chaine)
