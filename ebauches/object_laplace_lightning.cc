// BLENDER LAPLACIAN LIGHTNING;
//////////////////////////////////////////////////////// teldredge //////////////////////////////////////////////////////////////;
//////////////////////////////////////////////// www.funkboxing.com ////////////////////////////////////////////////////;
////////////////////////////////// https) {//developer.blender.org/T27189 //////////////////////////////;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
//////////////////////////////////////////////// using algorithm from ////////////////////////////////////////////////;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
//////////////////////////// FAST SIMULATION OF LAPLACIAN GROWTH (FSLG) ////////////////////////;
//////////////////////////////////////// http) {//gamma.cs.unc.edu/FRAC/ //////////////////////////////////////;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
//////////////////////////////////////////// and a few ideas ideas from ////////////////////////////////////////;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
////////// FAST ANIMATION OF LIGHTNING USING AN ADAPTIVE MESH (FALUAM) ////////;
//////////////////////////////// http) {//gamma.cs.unc.edu/FAST_LIGHTNING/ //////////////////////////;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
import bpy;
import time;
import random;
from math import sqrt;
from mathutils import Vector;
import struct;
import bisect;
import os.path;
notZero = 0.0000000001;
//scn = bpy.context.scene;
winmgr = bpy.context.window_manager;
;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
////////////////////////////////////////////////////// UTILITY FXNS //////////////////////////////////////////////////////////;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
template <typename T>;
auto within(T x, T y, T d);
{
	return (x - d <= y) && (x + d >= y);;
}

template <typename T>;
auto dist(ax, ay, az ,bx, by, bz);
{
    return longueur(dls) {) {math) {) {vec3<T>(ax, ay, ab) - dls) {) {math) {) {vec3<T>(bx,by,bz));;
}

auto splitList(aList, idx)
{
    auto ll = std::vector) {};
    for (auto &x : aList) {
        ll.push_back(x[idx]);
    }
    return ll;
}

auto splitListCo(aList)
{
    auto ll = std::vector) {};
    for (auto &p : aList) {
        ll.push_back((p[0], p[1], p[2]));
    }
    return ll;
}

auto getLowHigh(aList)
{
    auto tLow = aList[0];
    auto tHigh = aList[0];

    for (auto &a : aList) {
        if (a < tLow) {
			tLow = a;
		}
        if (a > tHigh) {
			 tHigh = a;
		}
    }

    return tLow, tHigh;
}

auto weightedRandomChoice(aList)
{
    auto tL = [];
    auto tweight = 0;

    for (auto &a : range(len(aList))) {
        auto idex = a;
        auto weight = aList[a];

        if (weight > 0.0) {
            tweight += weight;
            tL.push_back((tweight, idex));
        }
    }

    auto i = bisect.bisect(tL, (random.uniform(0, tweight), None));
    auto r = tL[i][1];
    return r;
}

auto getStencil3D_26(x,y,z)
{
    auto nL = [];
    for (auto &xT : range(x-1, x+2)) {
        for (auto &yT : range(y-1, y+2)) {
            for (auto &zT : range(z-1, z+2)) {
                nL.push_back((xT, yT, zT));
            }
        }
    }

    nL.remove((x,y,z));
    return nL;
}

auto jitterCells(aList, jit)
{
    j = jit/2;
    bList = [];

    for (auto &a : aList) {
        ax = a[0] + random.uniform(-j, j);
        ay = a[1] + random.uniform(-j, j);
        az = a[2] + random.uniform(-j, j);
        bList.push_back((ax, ay, az));
    }

    return bList;
}

auto deDupe(seq, idfun=None)
{
////////---THANKS TO THIS GUY - http) {//www.peterbe.com/plog/uniqifiers-benchmark;
    if (idfun is None) {
        auto idfun(x)) { return x; }
    }

    seen = {};
    result = [];
    for (auto &item : seq) {
        marker = idfun(item);
        if (marker : seen) {
			continue;
		}

        seen[marker] = 1;
        result.push_back(item);
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
////////////////////////////////////////////////// VISUALIZATION FXNS ////////////////////////////////////////////////////;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
auto writeArrayToVoxel(arr, filename)
{
    gridS = 64;
    half = int(gridS/2);
    bitOn = 255;
    aGrid = [[[0 for (auto &z : range(gridS)] for (auto &y : range(gridS)] for (auto &x : range(gridS)];

    for (auto &a : arr) {
        try) {
            aGrid[a[0]+half][a[1]+half][a[2]+half] = bitOn;
        }
        except) {
            print('Particle beyond voxel domain');
        }
    }
    file = open(filename, "wb");
    for (auto &z : range(gridS)) {
        for (auto &y : range(gridS)) {
            for (auto &x : range(gridS)) {
                file.write(struct.pack('B', aGrid[x][y][z]));
            }
        }
    }
    file.flush();
    file.close();
}

auto writeArrayToFile(arr, filename)
{
    file = open(filename, "w");
    for (auto &a : arr) {
        tstr = str(a[0]) + ',' + str(a[1]) + ',' + str(a[2]) + '\n';
        file.write(tstr);
    }

    file.close;
}

auto readArrayFromFile(filename)
{
    file = open(filename, "r");
    arr = [];
    for (auto &f : file) {
        pt = f[0) {-1].split(',');
        arr.push_back((int(pt[0]), int(pt[1]), int(pt[2])));
    }
    return arr;
}

auto makeMeshCube_OLD(msize)
{
    msize = msize/2;
    mmesh = bpy.data.meshes.new('q');
    mmesh.vertices.add(8);
    mmesh.vertices[0].co = [-msize, -msize, -msize];
    mmesh.vertices[1].co = [-msize,  msize, -msize];
    mmesh.vertices[2].co = [ msize,  msize, -msize];
    mmesh.vertices[3].co = [ msize, -msize, -msize];
    mmesh.vertices[4].co = [-msize, -msize,  msize];
    mmesh.vertices[5].co = [-msize,  msize,  msize];
    mmesh.vertices[6].co = [ msize,  msize,  msize];
    mmesh.vertices[7].co = [ msize, -msize,  msize];
    mmesh.faces.add(6);
    mmesh.faces[0].vertices_raw = [0,1,2,3];
    mmesh.faces[1].vertices_raw = [0,4,5,1];
    mmesh.faces[2].vertices_raw = [2,1,5,6];
    mmesh.faces[3].vertices_raw = [3,2,6,7];
    mmesh.faces[4].vertices_raw = [0,3,7,4];
    mmesh.faces[5].vertices_raw = [5,4,7,6];
    mmesh.update(calc_edges=true);
    return(mmesh);
;
auto makeMeshCube(msize)) {
    m2 = msize/2;
    //verts = [(0,0,0),(0,5,0),(5,5,0),(5,0,0),(0,0,5),(0,5,5),(5,5,5),(5,0,5)];
    verts = [(-m2,-m2,-m2),(-m2,m2,-m2),(m2,m2,-m2),(m2,-m2,-m2),;
             (-m2,-m2,m2),(-m2,m2,m2),(m2,m2,m2),(m2,-m2,m2)];
    faces = [(0,1,2,3), (4,5,6,7), (0,4,5,1), (1,5,6,2), (2,6,7,3), (3,7,4,0)];
;
    //Define mesh and object;
    mmesh = bpy.data.meshes.new("Cube");
    //mobject = bpy.data.objects.new("Cube", mmesh);
;
    //Set location and scene of object;
    //mobject.location = bpy.context.scene.cursor_location;
    //bpy.context.scene.objects.link(mobject);
;
    //Create mesh;
    mmesh.from_pydata(verts,[],faces);
    mmesh.update(calc_edges=true);
    return(mmesh);
;
auto writeArrayToCubes(arr, gridBU, orig, cBOOL = false, jBOOL = true)) {
    for (auto &a : arr) {
        x = a[0]; y = a[1]; z = a[2];
        me = makeMeshCube(gridBU);
        ob = bpy.data.objects.new('xCUBE', me);
        ob.location.x = (x*gridBU) + orig[0];
        ob.location.y = (y*gridBU) + orig[1];
        ob.location.z = (z*gridBU) + orig[2];
        if (cBOOL) { //////---!!!MOSTLY UNUSED;
            //////   POS+BLUE, NEG-RED, ZERO) {BLACK;
            col = (1.0, 1.0, 1.0, 1.0);
            if (a[3] == 0) { col = (0.0, 0.0, 0.0, 1.0);
            if (a[3] < 0) { col = (-a[3], 0.0, 0.0, 1.0);
            if (a[3] > 0) { col = (0.0, 0.0, a[3], 1.0);
            ob.color = col;
        bpy.context.scene.objects.link(ob);
        bpy.context.scene.update();
    if (jBOOL) {
        //////---SELECTS ALL CUBES w/ ?bpy.ops.object.join() b/c;
        //////   CAN'T JOIN ALL CUBES TO A SINGLE MESH RIGHT... ARGH...;
        for (auto &q : bpy.context.scene.objects) {
            q.select = false;
            if (q.name[0) {5] == 'xCUBE') {
                q.select = true;
                bpy.context.scene.objects.active = q;
;
auto addVert(ob, pt, conni = -1)) {
    mmesh = ob.data;
    mmesh.vertices.add(1);
    vcounti = len(mmesh.vertices)-1;
    mmesh.vertices[vcounti].co = [pt[0], pt[1], pt[2]];
    if (conni > -1) {
        mmesh.edges.add(1);
        ecounti = len(mmesh.edges)-1;
        mmesh.edges[ecounti].vertices = [conni, vcounti];
        mmesh.update();
;
auto addEdge(ob, va, vb)) {
    mmesh = ob.data;
    mmesh.edges.add(1);
    ecounti = len(mmesh.edges)-1;
    mmesh.edges[ecounti].vertices = [va, vb];
    mmesh.update();
;
auto newMesh(mname)) {
    mmesh = bpy.data.meshes.new(mname);
    omesh = bpy.data.objects.new(mname, mmesh);
    bpy.context.scene.objects.link(omesh);
    return omesh;
;
auto writeArrayToMesh(mname, arr, gridBU, rpt = None)) {
    mob = newMesh(mname);
    mob.scale = (gridBU, gridBU, gridBU);
    if (rpt) { addReportProp(mob, rpt);
    addVert(mob, arr[0], -1);
    for (auto &ai : range(1, len(arr))) {
        a = arr[ai];
        addVert(mob, a, ai-1);
    return mob;
;
//////---!!!OUT OF ORDER - SOME PROBLEM WITH IT ADDING (0,0,0);
auto writeArrayToCurves(cname, arr, gridBU, bd = .05, rpt = None)) {
    cur = bpy.data.curves.new('fslg_curve', 'CURVE');
    cur.use_fill_front = false;
    cur.use_fill_back = false;
    cur.bevel_depth = bd;
    cur.bevel_resolution = 2;
    cob = bpy.data.objects.new(cname, cur);
    cob.scale = (gridBU, gridBU, gridBU);
    if (rpt) { addReportProp(cob, rpt);
    bpy.context.scene.objects.link(cob);
    cur.splines.new('BEZIER');
    cspline = cur.splines[0];
    div = 1 //////   SPACING for (auto &HANDLES (2 - 1/2 WAY, 1 - NEXT BEZIER);
    for (auto &a : range(len(arr))) {
        cspline.bezier_points.add(1);
        bp = cspline.bezier_points[len(cspline.bezier_points)-1];
        if (a-1 < 0) { hL = arr[a];
        else) {
            hx = arr[a][0] - ((arr[a][0]-arr[a-1][0]) / div);
            hy = arr[a][1] - ((arr[a][1]-arr[a-1][1]) / div);
            hz = arr[a][2] - ((arr[a][2]-arr[a-1][2]) / div);
            hL = (hx,hy,hz);
;
        if (a+1 > len(arr)-1) { hR = arr[a];
        else) {
            hx = arr[a][0] + ((arr[a+1][0]-arr[a][0]) / div);
            hy = arr[a][1] + ((arr[a+1][1]-arr[a][1]) / div);
            hz = arr[a][2] + ((arr[a+1][2]-arr[a][2]) / div);
            hR = (hx,hy,hz);
        bp.co = arr[a];
        bp.handle_left = hL;
        bp.handle_right = hR;
;
auto addArrayToMesh(mob, arr)) {
    addVert(mob, arr[0], -1);
    mmesh = mob.data;
    vcounti = len(mmesh.vertices)-1;
    for (auto &ai : range(1, len(arr))) {
        a = arr[ai];
        addVert(mob, a, len(mmesh.vertices)-1);
;
auto addMaterial(ob, matname)) {
    mat = bpy.data.materials[matname];
    ob.active_material = mat;
;
auto writeStokeToMesh(arr, jarr, MAINi, HORDERi, TIPSi, orig, gs, rpt=None)) {
    //////---MAIN BRANCH;
    print('   WRITING MAIN BRANCH');
    llmain = [];
    for (auto &x : MAINi) {
        llmain.push_back(jarr[x]);
    mob = writeArrayToMesh('la0MAIN', llmain, gs);
    mob.location = orig;
;
    //////---hORDER BRANCHES;
    for (auto &hOi : range(len(HORDERi))) {
        print('   WRITING ORDER', hOi);
        hO = HORDERi[hOi];
        hob = newMesh('la1H'+str(hOi));
;
        for (auto &y : hO) {
            llHO = [];
            for (auto &x : y) {
                llHO.push_back(jarr[x]);
            addArrayToMesh(hob, llHO);
        hob.scale = (gs, gs, gs);
        hob.location = orig;
;
    //////---TIPS;
    print('   WRITING TIP PATHS');
    tob = newMesh('la2TIPS');
    for (auto &y :  TIPSi) {
        llt = [];
        for (auto &x : y) {
            llt.push_back(jarr[x]);
        addArrayToMesh(tob, llt);
    tob.scale = (gs, gs, gs);
    tob.location = orig;
;
    //////---ADD MATERIALS TO OBJECTS (if (THEY EXIST);
    try) {
        addMaterial(mob, 'edgeMAT-h0');
        addMaterial(hob, 'edgeMAT-h1');
        addMaterial(tob, 'edgeMAT-h2');
        print('   ADDED MATERIALS');
    except) { print('   MATERIALS NOT FOUND');
    //////---ADD GENERATION REPORT TO ALL MESHES;
    if (rpt) {
        addReportProp(mob, rpt);
        addReportProp(hob, rpt);
        addReportProp(tob, rpt);
;
auto writeStokeToSingleMesh(arr, jarr, orig, gs, mct, rpt=None)) {
    sgarr = buildCPGraph(arr, mct);
    llALL = [];
;
    Aob = newMesh('laALL');
    for (auto &pt : jarr) {
        addVert(Aob, pt);
    for (auto &cpi : range(len(sgarr))) {
        ci = sgarr[cpi][0];
        pi = sgarr[cpi][1];
        addEdge(Aob, pi, ci);
    Aob.location = orig;
    Aob.scale = ((gs,gs,gs));
;
    if (rpt) {
        addReportProp(Aob, rpt);
;
auto visualizeArray(cg, oob, gs, vm, vs, vc, vv, rst)) {
//////---IN) { (cellgrid, origin, gridscale,;
//////   mulimesh, single mesh, cubes, voxels, report sting);
    origin = oob.location;
;
    //////---DEAL WITH VERT MULTI-ORIGINS;
    oct = 2;
    if (oob.type == 'MESH') { oct = len(oob.data.vertices);
;
    //////---JITTER CELLS;
    if (vm or vs) { cjarr = jitterCells(cg, 1);
;
;
    if (vm) {  //////---WRITE ARRAY TO MULTI MESH;
;
        aMi, aHi, aTi = classifyStroke(cg, oct, winmgr.HORDER);
        print(') {) {) {WRITING TO MULTI-MESH');
        writeStokeToMesh(cg, cjarr, aMi, aHi, aTi, origin, gs, rst);
        print(') {) {) {MULTI-MESH WRITTEN');
;
    if (vs) {  //////---WRITE TO SINGLE MESH;
        print(') {) {) {WRITING TO SINGLE MESH');
        writeStokeToSingleMesh(cg, cjarr, origin, gs, oct, rst);
        print(') {) {) {SINGLE MESH WRITTEN');
;
    if (vc) {  //////---WRITE ARRAY TO CUBE OBJECTS;
        print(') {) {) {WRITING TO CUBES');
        writeArrayToCubes(cg, gs, origin);
        print(') {) {) {CUBES WRITTEN');
;
    if (vv) {  //////---WRITE ARRAY TO VOXEL DATA FILE;
        print(') {) {) {WRITING TO VOXELS');
        fname = "FSLGvoxels.raw";
        path = os.path.dirname(bpy.data.filepath);
        writeArrayToVoxel(cg, path + "\\" + fname);
        print(') {) {) {VOXEL DATA WRITTEN TO - ', path + "\\" + fname);
;
    //////---READ/WRITE ARRAY TO FILE (MIGHT NOT BE NECESSARY);
    //tfile = 'c) {\\testarr.txt';
    //writeArrayToFile(cg, tfile);
    //cg = readArrayFromFile(tfile);
;
    //////---READ/WRITE ARRAY TO CURVES (OUT OF ORDER);
    //writeArrayToCurves('laMAIN', llmain, .10, .25);
;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
////////////////////////////////////////////////////// ALGORITHM FXNS //////////////////////////////////////////////////////;
//////////////////////////////////////////////////// FROM FALUAM PAPER //////////////////////////////////////////////////;
//////////////////////////////////////////// PLUS SOME STUFF I MADE UP //////////////////////////////////////////;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
auto buildCPGraph(arr, sti = 2)
{
////////---IN -XYZ ARRAY AS BUILT BY GENERATOR;
////////---OUT -[(CHILDindex, PARENTindex)];
////////   sti - start index, 2 for (auto &Empty, len(me.vertices) for (auto &Mesh;
    sgarr = [];
    sgarr.push_back((1, 0)) //;
    for (auto &ai : range(sti, len(arr))) {
        cs = arr[ai];
        cpts = arr[0 : ai];
        cslap = getStencil3D_26(cs[0], cs[1], cs[2]);
;
        for (auto &nc : cslap) {
            ct = cpts.count(nc);
            if (ct>0) {
                cti = cpts.index(nc);
            }
        }
        sgarr.push_back((ai, cti));
    }
    return sgarr;
}

auto buildCPGraph_WORKINPROGRESS(arr, sti = 2)
{
////////---IN -XYZ ARRAY AS BUILT BY GENERATOR;
////////---OUT -[(CHILDindex, PARENTindex)];
////////   sti - start index, 2 for (auto &Empty, len(me.vertices) for (auto &Mesh;
    sgarr = [];
    sgarr.push_back((1, 0)) //;
    ctix = 0;
    for (auto &ai : range(sti, len(arr))) {
        cs = arr[ai];
        //cpts = arr[0:ai];
        cpts = arr[ctix:ai];
        cslap = getStencil3D_26(cs[0], cs[1], cs[2]);
        for (auto &nc : cslap) {
            ct = cpts.count(nc);
            if (ct>0) {
                //cti = cpts.index(nc);
                cti = ctix + cpts.index(nc);
                ctix = cpts.index(nc);
            }
        }
        sgarr.push_back((ai, cti));
    }
    return sgarr;
}

auto findChargePath(oc, fc, ngraph, restrict = [], partial = true)
{
    //////---oc -ORIGIN CHARGE INDEX, fc -FINAL CHARGE INDEX;
    //////---ngraph -NODE GRAPH, restrict- INDEX OF SITES CANNOT TRAVERSE;
    //////---partial -RETURN PARTIAL PATH if (RESTRICTION ENCOUNTERD;
    cList = splitList(ngraph, 0);
    pList = splitList(ngraph, 1);
    aRi = [];
    cNODE = fc;
    for (auto &x : range(len(ngraph)))) {
        pNODE = pList[cList.index(cNODE)];
        aRi.push_back(cNODE);
        cNODE = pNODE;
        npNODECOUNT = cList.count(pNODE);
        if (cNODE == oc) {             //////   STOP if (ORIGIN FOUND;
            aRi.push_back(cNODE)       //////   RETURN PATH;
            return aRi;
        }
        if (npNODECOUNT == 0) {        //////   STOP if (NO PARENTS;
            return []
        }         //////   RETURN [];
        if (pNODE : restrict) {       //////   STOP if (PARENT IS IN RESTRICTION;
            if (partial) {             //////   RETURN PARTIAL OR [];
                aRi.push_back(cNODE);
                return aRi;
            }
            else) { return []; }
        }
    }
}
;
auto findTips(arr)
{
    lt = [];
    for (auto &ai : arr[0) {len(arr)-1]) {
        a = ai[0];
        cCOUNT = 0;
        for (auto &bi : arr) {
            b = bi[1];
            if (a == b) {
                cCOUNT += 1;
            }
        }
        if (cCOUNT == 0) {
            lt.push_back(a);
        }
    }
    return lt;
}

auto findChannelRoots(path, ngraph, restrict = [])
{
    roots = [];
    for (auto &ai : range(len(ngraph))) {
        chi = ngraph[ai][0];
        par = ngraph[ai][1];
        if (par : path and not chi : path and
            not chi : restrict) {
            roots.push_back(par);
        }
    }
    droots = deDupe(roots);
    return droots;
}

auto findChannels(roots, tips, ngraph, restrict)
{
    cPATHS = [];
    for (auto &ri : range(len(roots))) {
        r = roots[ri];
        sL = 1;
        sPATHi = [];
        for (auto &ti : range(len(tips))) {
            t = tips[ti];
            if (t < r) {
				continue;
			}
            tPATHi = findChargePath(r, t, ngraph, restrict, false);
            tL = len(tPATHi);
            if (tL > sL) {
                if (countChildrenOnPath(tPATHi, ngraph) > 1) {
                    sL = tL;
                    sPATHi = tPATHi;
                    tTEMP = t; tiTEMP = ti;
                }
            }
        }
        if (len(sPATHi) > 0) {
            print('   found path/idex from', ri, 'of',;
                  len(roots), 'possible | tips) {', tTEMP, tiTEMP);
            cPATHS.push_back(sPATHi);
            tips.remove(tTEMP);
        }
    }
    return cPATHS;
}

auto findChannels_WORKINPROGRESS(roots, ttips, ngraph, restrict)
{
    cPATHS = [];
    tips = list(ttips);
    for (auto &ri : range(len(roots))) {
        r = roots[ri];
        sL = 1;
        sPATHi = [];
        tipREMOVE = [] //////---CHECKED TIP INDEXES, TO BE REMOVED for (auto &NEXT LOOP;
        for (auto &ti : range(len(tips))) {
            t = tips[ti];
            //print('-CHECKING RT/IDEX) {', r, ri, 'AGAINST TIP', t, ti);
            //if (t < r) { continue;
            if (ti < ri) { continue;}
            tPATHi = findChargePath(r, t, ngraph, restrict, false);
            tL = len(tPATHi);
            if (tL > sL) {
                if (countChildrenOnPath(tPATHi, ngraph) > 1) {
                    sL = tL;
                    sPATHi = tPATHi;
                    tTEMP = t; tiTEMP = ti;
                }
            }

            if (tL > 0) {
                tipREMOVE.push_back(t);
            }
        }
        if (len(sPATHi) > 0) {
            print('   found path from root idex', ri, 'of',;
                   len(roots), 'possible roots | //oftips=', len(tips));
            cPATHS.push_back(sPATHi);
        }
        for (auto &q : tipREMOVE) {
			tips.remove(q);
		}
	}
    return cPATHS;
}

auto countChildrenOnPath(aPath, ngraph, quick = true)
{
    //////---RETURN HOW MANY BRANCHES;
    //////   COUNT WHEN NODE IS A PARENT >1 TIMES;
    //////   quick -STOP AND RETURN AFTER FIRST;
    cCOUNT = 0;
    pList = splitList(ngraph,1);
    for (auto &ai : range(len(aPath)-1)) {
        ap = aPath[ai];
        pc = pList.count(ap);
        if (quick and pc > 1) {
            return pc;
        }
    }
    return cCOUNT;
}

//////---CLASSIFY CHANNELS INTO 'MAIN', 'hORDER/SECONDARY' and 'SIDE';
auto classifyStroke(sarr, mct, hORDER = 1)
{
    print(') {) {) {CLASSIFYING STROKE');
    //////---BUILD CHILD/PARENT GRAPH (INDEXES OF sarr);
    sgarr = buildCPGraph(sarr, mct);
;
    //////---FIND MAIN CHANNEL;
    print('   finding MAIN');
    oCharge = sgarr[0][1];
    fCharge = sgarr[len(sgarr)-1][0];
    aMAINi = findChargePath(oCharge, fCharge, sgarr);
;
    //////---FIND TIPS;
    print('   finding TIPS');
    aTIPSi = findTips(sgarr);
;
    //////---FIND hORDER CHANNEL ROOTS;
    //////   hCOUNT = ORDERS BEWTEEN MAIN and SIDE/TIPS;
    //////   !!!STILL BUGGY!!!;
    hRESTRICT = list(aMAINi)    ////// ADD TO THIS AFTER EACH TIME;
    allHPATHSi = []             ////// ALL hO PATHS) { [[h0], [h1]...];
    curPATHSi = [aMAINi]        ////// LIST OF PATHS FIND ROOTS ON;
    for (auto &h : range(hORDER)) {
        allHPATHSi.push_back([]);
        for (auto &pi : range(len(curPATHSi))) {     //////   LOOP THROUGH ALL PATHS IN THIS ORDER;
            p = curPATHSi[pi];
            //////   GET ROOTS for (auto &THIS PATH;
            aHROOTSi = findChannelRoots(p, sgarr, hRESTRICT);
            print('   found', len(aHROOTSi), 'roots : ORDER', h, ') {//paths) {', len(curPATHSi));
            ////// GET CHANNELS for (auto &THESE ROOTS;
            if (len(aHROOTSi) == 0) {
                print('NO ROOTS for (auto &FOUND for (auto &CHANNEL');
                aHPATHSi = [];
                continue;
            }
            else) {
                aHPATHSiD = findChannels(aHROOTSi, aTIPSi, sgarr, hRESTRICT);
                aHPATHSi = aHPATHSiD;
                allHPATHSi[h] += aHPATHSi;
                //////   SET THESE CHANNELS AS RESTRICTIONS for (auto &NEXT ITERATIONS;
                for (auto &hri : aHPATHSi) {
                    hRESTRICT += hri;

                }
            }
        }
        curPATHSi = aHPATHSi;
	}

    //////---SIDE BRANCHES, FINAL ORDER OF HEIRARCHY;
    //////   FROM TIPS THAT ARE NOT IN AN EXISTING PATH;
    //////   BACK TO ANY OTHER POINT THAT IS ALREADY ON A PATH;
    aDRAWNi = [];
    aDRAWNi += aMAINi;
    for (auto &oH : allHPATHSi) {
        for (auto &o : oH) {
            aDRAWNi += o;
        }
    }

    aTPATHSi = [];
    for (auto &a : aTIPSi) {
        if (not a : aDRAWNi) {
            aPATHi = findChargePath(oCharge, a, sgarr, aDRAWNi);
            aDRAWNi += aPATHi;
            aTPATHSi.push_back(aPATHi);
        }
    }

    return aMAINi, allHPATHSi, aTPATHSi;
}

auto voxelByVertex(ob, gs)
{
//////---'VOXELIZES' VERTS IN A MESH TO LIST [(x,y,z),(x,y,z)];
//////   W/ RESPECT GSCALE AND OB ORIGIN (B/C SHOULD BE ORIGIN OBJ);
    orig = ob.location;
    ll = [];
    for (auto &v : ob.data.vertices) {
        x = int( v.co.x / gs );
        y = int( v.co.y / gs );
        z = int( v.co.z / gs );
        ll.push_back((x,y,z));
    }

    return ll;
}

auto voxelByRays(ob, orig, gs)
{
//////--- MESH INTO A 3DGRID W/ RESPECT GSCALE AND BOLT ORIGIN;
//////   -DOES NOT TAKE OBJECT ROTATION/SCALE INTO ACCOUNT;
//////   -THIS IS A HORRIBLE, INEFFICIENT FUNCTION;
//////    MAYBE THE RAYCAST/GRID THING ARE A BAD IDEA. BUT I;
//////    HAVE TO 'VOXELIZE THE OBJECT W/ RESCT TO GSCALE/ORIGIN;
    bbox = ob.bound_box;
    bbxL = bbox[0][0]; bbxR = bbox[4][0];
    bbyL = bbox[0][1]; bbyR = bbox[2][1];
    bbzL = bbox[0][2]; bbzR = bbox[1][2];
    xct = int((bbxR - bbxL) / gs);
    yct = int((bbyR - bbyL) / gs);
    zct = int((bbzR - bbzL) / gs);
    xs = int(xct/2); ys = int(yct/2); zs = int(zct/2);
    print('  CASTING', xct, '/', yct, '/', zct, 'cells, total) {', xct*yct*zct, 'in obj-', ob.name);
    ll = [];
    rc = 100    //////---DISTANCE TO CAST FROM;
    //////---RAYCAST TOP/BOTTOM;
    print('  RAYCASTING TOP/BOTTOM');
    for (auto &x : range(xct)) {
        for (auto &y : range(yct)) {
            xco = bbxL + (x*gs);  yco = bbyL + (y*gs);
            v1 = ((xco, yco,  rc));    v2 = ((xco, yco, -rc));
            vz1 = ob.ray_cast(v1,v2);   vz2 = ob.ray_cast(v2,v1);
            if (vz1[2] != -1) {
				ll.push_back((x-xs, y-ys, int(vz1[0][2] * (1/gs)) ));
			}
            if (vz2[2] != -1) {
				ll.push_back((x-xs, y-ys, int(vz2[0][2] * (1/gs)) ));
			}
		}
    }
    //////---RAYCAST FRONT/BACK;
    print('  RAYCASTING FRONT/BACK');
    for (auto &x : range(xct)) {
        for (auto &z : range(zct)) {
            xco = bbxL + (x*gs);  zco = bbzL + (z*gs);
            v1 = ((xco, rc,  zco));    v2 = ((xco, -rc, zco));
            vy1 = ob.ray_cast(v1,v2);   vy2 = ob.ray_cast(v2,v1);
            if (vy1[2] != -1) {
				ll.push_back((x-xs, int(vy1[0][1] * (1/gs)), z-zs));
            }
            if (vy2[2] != -1) {
				ll.push_back((x-xs, int(vy2[0][1] * (1/gs)), z-zs));
			}
    	}
    }

    //////---RAYCAST LEFT/RIGHT;
    print('  RAYCASTING LEFT/RIGHT');
    for (auto &y : range(yct)) {
        for (auto &z : range(zct)) {
            yco = bbyL + (y*gs);  zco = bbzL + (z*gs);
            v1 = ((rc, yco,  zco));    v2 = ((-rc, yco, zco));
            vx1 = ob.ray_cast(v1,v2);   vx2 = ob.ray_cast(v2,v1);
            if (vx1[2] != -1) {
				ll.push_back((int(vx1[0][0] * (1/gs)), y-ys, z-zs));
            }
            if (vx2[2] != -1) {
				ll.push_back((int(vx2[0][0] * (1/gs)), y-ys, z-zs));
			}
		}
    }

    //////---ADD IN NEIGHBORS SO BOLT WONT GO THRU;
    nlist = [];
    for (auto &l : ll) {
        nl = getStencil3D_26(l[0], l[1], l[2]);
        nlist += nl;
	}

    //////---DEDUPE;
    print('  ADDED NEIGHBORS, DEDUPING...');
    rlist = deDupe(ll+nlist);
    qlist = [];
;
    //////---RELOCATE GRID W/ RESPECT GSCALE AND BOLT ORIGIN;
    //////   !!!NEED TO ADD IN OBJ ROT/SCALE HERE SOMEHOW...;
    od = Vector(( (ob.location[0] - orig[0]) / gs,;
                  (ob.location[1] - orig[1]) / gs,;
                  (ob.location[2] - orig[2]) / gs ));
    for (auto &r : rlist) {
        qlist.push_back((r[0]+int(od[0]), r[1]+int(od[1]), r[2]+int(od[2]) ));
	}

    return qlist;
}

auto fakeGroundChargePlane(z, charge)
{
    eCL = [];
    xy = abs(z)/2;
    eCL += [(0, 0, z, charge)];
    eCL += [(xy, 0, z, charge)];
    eCL += [(0, xy, z, charge)];
    eCL += [(-xy, 0, z, charge)];
    eCL += [(0, -xy, z, charge)];
    return eCL;
}

auto addCharges(ll, charge)
{
//////---IN) { ll - [(x,y,z), (x,y,z)], charge - w;
//////   OUT clist - [(x,y,z,w), (x,y,z,w)];
    clist = [];
    for (auto &l : ll) {
        clist.push_back((l[0], l[1], l[2], charge));
	}
    return clist;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
////////////////////////////////////////////////////// ALGORITHM FXNS //////////////////////////////////////////////////////;
//////////////////////////////////////////////////////////// FROM FSLG //////////////////////////////////////////////////////////;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
auto getGrowthProbability_KEEPFORREFERENCE(uN, aList)
{
    //////---IN) { uN -USER TERM, cList -CANDIDATE SITES, oList -CANDIDATE SITE CHARGES;
    //////   OUT) { LIST OF [(XYZ), POT, PROB];
    cList = splitList(aList, 0);
    oList = splitList(aList, 1);
    Omin, Omax = getLowHigh(oList);
    if (Omin == Omax) { Omax += notZero; Omin -= notZero;
    PdL = [];
    E = 0;
    E = notZero   //////===DIVISOR for (auto &(FSLG - Eqn. 12);
    for (auto &o : oList) {
        Uj = (o - Omin) / (Omax - Omin) //////===(FSLG - Eqn. 13);
        E += pow(Uj, uN);
    }
    for (auto &oi : range(len(oList))) {
        o = oList[oi];
        Ui = (o - Omin) / (Omax - Omin);
        Pd = (pow(Ui, uN)) / E //////===(FSLG - Eqn. 12);
        PdINT = Pd * 100;
        PdL.push_back(Pd);
    }
    return PdL;
}

//////---WORK IN PROGRESS, TRYING TO SPEED THESE UP;
auto fslg_e13(x, min, max, u)
{
	return pow((x - min) / (max - min), u);
}

auto addit(x,y)
{
	return x+y;
}

auto fslg_e12(x, min, max, u, e)
{
	return (fslg_e13(x, min, max, u) / e) * 100;
}

auto getGrowthProbability(uN, aList)
{
    //////---IN) { uN -USER TERM, cList -CANDIDATE SITES, oList -CANDIDATE SITE CHARGES;
    //////   OUT) { LIST OF PROB;
    cList = splitList(aList, 0);
    oList = splitList(aList, 1);
    Omin, Omax = getLowHigh(oList);
    if (Omin == Omax) {
		Omax += notZero; Omin -= notZero;
	}

    PdL = [];
    E = notZero;
    minL = [Omin for (auto &q : range(len(oList))];
    maxL = [Omax for (auto &q : range(len(oList))];
    uNL =  [uN   for (auto &q : range(len(oList))];
    E = sum(map(fslg_e13, oList, minL, maxL, uNL));
    EL = [E for (auto &q : range(len(oList))];
    mp = map(fslg_e12, oList, minL, maxL, uNL, EL);
    for (auto &m : mp) {
		PdL.push_back(m);
	}
    return PdL;
}

auto updatePointCharges(p, cList, eList = [])
{
    //////---IN) { pNew -NEW GROWTH CELL;
    //////       cList -OLD CANDIDATE SITES, eList -SAME;
    //////   OUT) { LIST OF NEW CHARGE AT CANDIDATE SITES;
    r1 = 1/2        //////===(FSLG - Eqn. 10);
    nOiL = [];
    for (auto &oi : range(len(cList))) {
        o = cList[oi][1];
        c = cList[oi][0];
        iOe = 0;
        rit = dist(c[0], c[1], c[2], p[0], p[1], p[2]);
        iOe += (1 - (r1/rit));
        Oit =  o + iOe;
        nOiL.push_back((c, Oit));
    }

    return nOiL;
}

auto initialPointCharges(pList, cList, eList = [])
{
    //////---IN) { p -CHARGED CELL (XYZ), cList -CANDIDATE SITES (XYZ, POT, PROB);
    //////   OUT) { cList -WITH POTENTIAL CALCULATED;
    r1 = 1/2        //////===(FSLG - Eqn. 10);
    npList = [];
    for (auto &p : pList) {
        npList.push_back(((p[0], p[1], p[2]), 1.0));
    }

    for (auto &e : eList) {
        npList.push_back(((e[0], e[1], e[2]), e[3]));
    }

    OiL = [];
    for (auto &i : cList) {
        Oi = 0;

        for (auto &j : npList) {
            if (i != j[0]) {
                rij = dist(i[0], i[1], i[2], j[0][0], j[0][1], j[0][2]);
                Oi += (1 - (r1 / rij)) * j[1] ////// CHARGE INFLUENCE;
			}
        }
        OiL.push_back(((i[0], i[1], i[2]), Oi));
    }

    return OiL;
}

auto getCandidateSites(aList, iList = [])
{
    //////---IN) { aList -(X,Y,Z) OF CHARGED CELL SITES, iList -insulator sites;
    //////   OUT) { CANDIDATE LIST OF GROWTH SITES [(X,Y,Z)];
    tt1 = time.clock();
    cList = [];
    for (auto &c : aList) {
        tempList = getStencil3D_26(c[0], c[1], c[2]);
        for (auto &t : tempList) {
            if (not t : aList and not t : iList) {
                cList.push_back(t);
            }
        }
    }

    ncList = deDupe(cList);
    tt2 = time.clock();
    //print('FXNTIMER) {getCandidateSites) {', tt2-tt1, 'check 26 against) {', len(aList)+len(iList));
    return ncList;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
////////////////////////////////////////////////////////// SETUP FXNS //////////////////////////////////////////////////////////;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
auto setupObjects()
{
    //if (winmgr.OOB == "" or winmgr.OOB.name not : scene...;
    oOB = bpy.data.objects.new('ELorigin', None);
    oOB.location = ((0,0,10));
    bpy.context.scene.objects.link(oOB);
;
    gOB = bpy.data.objects.new('ELground', None);
    gOB.empty_draw_type = 'ARROWS';
    bpy.context.scene.objects.link(gOB);
;
    cME = makeMeshCube(1);
    cOB = bpy.data.objects.new('ELcloud', cME);
    cOB.location = ((-2,8,12));
    cOB.hide_render = true;
    bpy.context.scene.objects.link(cOB);
;
    iME = makeMeshCube(1);
    for (auto &v : iME.vertices) {
        xyl = 6.5; zl = .5;
        v.co[0] = v.co[0] * xyl;
        v.co[1] = v.co[1] * xyl;
        v.co[2] = v.co[2] * zl;
    }

    iOB = bpy.data.objects.new('ELinsulator', iME);
    iOB.location = ((0,0,5));
    iOB.hide_render = true;
    bpy.context.scene.objects.link(iOB);
;
    try) {
        winmgr.OOB = 'ELorigin';
        winmgr.GOB = 'ELground';
        winmgr.COB = 'ELcloud';
        winmgr.IOB = 'ELinsulator';
    except) {
		pass;
	}
}

auto checkSettings()
{
    check = true;

    if (winmgr.OOB == "") {
        print('ERROR) { NO ORIGIN OBJECT SELECTED');
        check = false;
    }

    if (winmgr.GROUNDBOOL and winmgr.GOB == "") {
        print('ERROR) { NO GROUND OBJECT SELECTED');
        check = false;
    }

    if (winmgr.CLOUDBOOL and winmgr.COB == "") {
        print('ERROR) { NO CLOUD OBJECT SELECTED');
        check = false;
    }

    if (winmgr.IBOOL and winmgr.IOB == "") {
        print('ERROR) { NO INSULATOR OBJECT SELECTED');
        check = false;
    }
    //should make a popup here;
    return check;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
////////////////////////////////////////////////////////////// MAIN //////////////////////////////////////////////////////////////////;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
auto FSLG()
{
//////======FAST SIMULATION OF LAPLACIAN GROWTH======//////;
    print('\n<<<<<<------GO GO GADGET) { FAST SIMULATION OF LAPLACIAN GROWTH!');
    tc1 = time.clock();
    TSTEPS = winmgr.TSTEPS;

    //obORIGIN = scn.objects[winmgr.OOB];
    //obGROUND = scn.objects[winmgr.GOB];
    obORIGIN = bpy.context.scene.objects[winmgr.OOB];
    obGROUND = bpy.context.scene.objects[winmgr.GOB];
    winmgr.ORIGIN = obORIGIN.location;
    winmgr.GROUNDZ = int((obGROUND.location[2] - winmgr.ORIGIN[2]) / winmgr.GSCALE);

    //////====== 1) INSERT INTIAL CHARGE(S) POINT (USES VERTS if (MESH);
    cgrid = [(0, 0, 0)];
    if (obORIGIN.type == 'MESH') {
        print("<<<<<<------ORIGIN OBJECT IS MESH, 'VOXELIZING' INTIAL CHARGES FROM VERTS");
        cgrid = voxelByVertex(obORIGIN, winmgr.GSCALE);
        if (winmgr.VMMESH) {
            print("<<<<<<------CANNOT CLASSIFY STROKE FROM VERT ORIGINS YET, NO MULTI-MESH OUTPUT");
            winmgr.VMMESH = false; winmgr.VSMESH = true;
		}
	}

    //////---GROUND CHARGE CELL / INSULATOR LISTS (eChargeList/icList);
    eChargeList = []; icList = [];
    if (winmgr.GROUNDBOOL) {
        eChargeList = fakeGroundChargePlane(winmgr.GROUNDZ, winmgr.GROUNDC);
    }
    if (winmgr.CLOUDBOOL) {
        print("<<<<<<------'VOXELIZING' CLOUD OBJECT (COULD TAKE SOME TIME)");
        obCLOUD = bpy.context.scene.objects[winmgr.COB];
        eChargeListQ = voxelByRays(obCLOUD, winmgr.ORIGIN, winmgr.GSCALE);
        eChargeList = addCharges(eChargeListQ, winmgr.CLOUDC);
        print('<<<<<<------CLOUD OBJECT CELL COUNT = ', len(eChargeList) );
    }
    if (winmgr.IBOOL) {
        print("<<<<<<------'VOXELIZING' INSULATOR OBJECT (COULD TAKE SOME TIME)");
        obINSULATOR = bpy.context.scene.objects[winmgr.IOB];
        icList = voxelByRays(obINSULATOR, winmgr.ORIGIN, winmgr.GSCALE);
        print('<<<<<<------INSULATOR OBJECT CELL COUNT = ', len(icList) );
        //writeArrayToCubes(icList, winmgr.GSCALE, winmgr.ORIGIN);
        //return 'THEEND';
	}
    //////====== 2) LOCATE CANDIDATE SITES AROUND CHARGE;
    cSites = getCandidateSites(cgrid, icList);

    //////====== 3) CALC POTENTIAL AT EACH SITE (Eqn. 10);
    cSites = initialPointCharges(cgrid, cSites, eChargeList);

    ts = 1;
    while (ts <= TSTEPS) {
        //////====== 1) SELECT NEW GROWTH SITE (Eqn. 12);
        //////===GET PROBABILITIES AT CANDIDATE SITES;
        gProbs = getGrowthProbability(winmgr.BIGVAR, cSites);
        //////===CHOOSE NEW GROWTH SITE BASED ON PROBABILITIES;
        gSitei = weightedRandomChoice(gProbs);
        gsite  = cSites[gSitei][0];

        //////====== 2) ADD NEW POINT CHARGE AT GROWTH SITE;
        //////===ADD NEW GROWTH CELL TO GRID;
        cgrid.push_back(gsite);
        //////===REMOVE NEW GROWTH CELL FROM CANDIDATE SITES;
        cSites.remove(cSites[gSitei]);

        //////====== 3) UPDATE POTENTIAL AT CANDIDATE SITES (Eqn. 11);
        cSites = updatePointCharges(gsite, cSites, eChargeList);

        //////====== 4) ADD NEW CANDIDATES SURROUNDING GROWTH SITE;
        //////===GET CANDIDATE 'STENCIL';
        ncSitesT = getCandidateSites([gsite], icList);
        //////===REMOVE CANDIDATES ALREADY IN CANDIDATE LIST OR CHARGE GRID;
        ncSites = [];
        cSplit = splitList(cSites, 0);
        for (auto &cn : ncSitesT) {
            if (not cn : cSplit and
            not cn : cgrid) {
                ncSites.push_back((cn, 0));
            }
		}
        //////====== 5) CALC POTENTIAL AT NEW CANDIDATE SITES (Eqn. 10);
        ncSplit = splitList(ncSites, 0);
        ncSites = initialPointCharges(cgrid, ncSplit, eChargeList);
;
        //////===ADD NEW CANDIDATE SITES TO CANDIDATE LIST;
        for (auto &ncs : ncSites) {
            cSites.push_back(ncs);
		}

        //////===ITERATION COMPLETE;
        istr1 = ') {) {) {T-STEP) { ' + str(ts) + '/' + str(TSTEPS);
        istr12 = ' | GROUNDZ) { ' + str(winmgr.GROUNDZ) + ' | ';
        istr2 = 'CANDS) { ' + str(len(cSites)) + ' | ';
        istr3 = 'GSITE) { ' + str(gsite);
        print(istr1 + istr12 + istr2 + istr3);
        ts += 1;
;
        //////---EARLY TERMINATION for (auto &GROUND/CLOUD STRIKE;
        if (winmgr.GROUNDBOOL) {
            if (gsite[2] == winmgr.GROUNDZ) {
                ts = TSTEPS+1;
                print('<<<<<<------EARLY TERMINATION DUE TO GROUNDSTRIKE');
                continue;
            }
        }

        if (winmgr.CLOUDBOOL) {
            //if (gsite : cloudList) {
            if (gsite : splitListCo(eChargeList)) {
                ts = TSTEPS+1;
                print('<<<<<<------EARLY TERMINATION DUE TO CLOUDSTRIKE');
                continue;
            }
        }
    }

    tc2 = time.clock();
    tcRUN = tc2 - tc1;
    print('<<<<<<------LAPLACIAN GROWTH LOOP COMPLETED) { ' + str(len(cgrid)) + ' / ' + str(tcRUN)[0) {5] + ' SECONDS');
    print('<<<<<<------VISUALIZING DATA');

    reportSTRING = getReportString(tcRUN);
    //////---VISUALIZE ARRAY;
    visualizeArray(cgrid, obORIGIN, winmgr.GSCALE, winmgr.VMMESH, winmgr.VSMESH, winmgr.VCUBE, winmgr.VVOX, reportSTRING);
    print('<<<<<<------COMPLETE');
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
//////////////////////////////////////////////////////////////// GUI //////////////////////////////////////////////////////////////////;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;
//////---NOT IN UI;
bpy.types.WindowManager.ORIGIN = bpy.props.FloatVectorProperty(name = "origin charge");
bpy.types.WindowManager.GROUNDZ = bpy.props.IntProperty(name = "ground Z coordinate");
bpy.types.WindowManager.HORDER = bpy.props.IntProperty(name = "secondary paths orders");
//////---IN UI;
bpy.types.WindowManager.TSTEPS = bpy.props.IntProperty(;
    name = "iterations", description = "number of cells to create, will end early if (hits ground plane or cloud");
bpy.types.WindowManager.GSCALE = bpy.props.FloatProperty(;
    name = "grid unit size", description = "scale of cells, .25 = 4 cells per blenderUnit");
bpy.types.WindowManager.BIGVAR = bpy.props.FloatProperty(;
    name = "straightness", description = "straightness/branchiness of bolt, <2 is mush, >12 is staight line, 6.3 is good");
bpy.types.WindowManager.GROUNDBOOL = bpy.props.BoolProperty(;
    name = "use ground object", description = "use ground plane or not");
bpy.types.WindowManager.GROUNDC = bpy.props.IntProperty(;
    name = "ground charge", description = "charge of ground plane");
bpy.types.WindowManager.CLOUDBOOL = bpy.props.BoolProperty(;
    name = "use cloud object", description = "use cloud obj, attracts and terminates like ground but any obj instead of z plane, can slow down loop if (obj is large, overrides ground");
bpy.types.WindowManager.CLOUDC = bpy.props.IntProperty(;
    name = "cloud charge", description = "charge of a cell : cloud object (so total charge also depends on obj size)");
;
bpy.types.WindowManager.VMMESH = bpy.props.BoolProperty(;
    name = "multi mesh", description = "output to multi-meshes for (auto &different materials on main/sec/side branches");
bpy.types.WindowManager.VSMESH = bpy.props.BoolProperty(;
    name = "single mesh", description = "output to single mesh for (auto &using build modifier and particles for (auto &effects");
bpy.types.WindowManager.VCUBE = bpy.props.BoolProperty(;
    name = "cubes", description = "CTRL-J after run to JOIN, outputs a bunch of cube objest, mostly for (auto &testing");
bpy.types.WindowManager.VVOX = bpy.props.BoolProperty(;
    name = "voxel (experimental)", description = "output to a voxel file to bpy.data.filepath\FSLGvoxels.raw - doesn't work well right now");
bpy.types.WindowManager.IBOOL = bpy.props.BoolProperty(;
    name = "use insulator object", description = "use insulator mesh object to prevent growth of bolt in areas");
bpy.types.WindowManager.OOB = bpy.props.StringProperty(description = "origin of bolt, can be an Empty, if (obj is mesh will use all verts as charges");
bpy.types.WindowManager.GOB = bpy.props.StringProperty(description = "object to use as ground plane, uses z coord only");
bpy.types.WindowManager.COB = bpy.props.StringProperty(description = "object to use as cloud, best to use a cube");
bpy.types.WindowManager.IOB = bpy.props.StringProperty(description = "object to use as insulator, 'voxelized' before generating bolt, can be slow");
;
//////---DEFAULT USER SETTINGS;
winmgr.TSTEPS = 350;
winmgr.HORDER = 1;
winmgr.GSCALE = 0.12;
winmgr.BIGVAR = 6.3;
winmgr.GROUNDBOOL = true;
winmgr.GROUNDC = -250;
winmgr.CLOUDBOOL = false;
winmgr.CLOUDC = -1;
winmgr.VMMESH = true;
winmgr.VSMESH = false;
winmgr.VCUBE = false;
winmgr.VVOX = false;
winmgr.IBOOL = false;
    winmgr.OOB = "ELorigin";
    winmgr.GOB = "ELground";
    winmgr.COB = "ELcloud";
    winmgr.IOB = "ELinsulator";
