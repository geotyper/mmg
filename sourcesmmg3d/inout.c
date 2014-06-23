#include "mmg3d.h"

/** read mesh data */
int loadMesh(pMesh mesh) {
  pTetra       pt;
  pTria        pt1;
  pEdge        pa;
  pPoint       ppt;
  float        fp1,fp2,fp3;
  int          i,k,inm,ia,np,nr,nre,nc,aux,nt,ref,v[3],na,*ina,nereq;
  char        *ptr,*name,data[128];

  name = mesh->namein;
  strcpy(data,name);
  ptr = strstr(data,".mesh");
  if ( !ptr ) {
    /* data contains the filename without extension */
    strcat(data,".meshb");
    if( !(inm = GmfOpenMesh(data,GmfRead,&mesh->ver,&mesh->dim)) ) {
      /* our file is not a .meshb file, try with .mesh ext */
      ptr = strstr(data,".mesh");
      *ptr = '\0';
      strcat(data,".mesh");
      if( !(inm = GmfOpenMesh(data,GmfRead,&mesh->ver,&mesh->dim)) ) {
        fprintf(stderr,"  ** %s  NOT FOUND.\n",data);
        return(0);
      }
      else if ( !strstr(mesh->nameout,".mesh") ) {
        ADD_MEM(mesh,5*sizeof(char),"output file name",
                printf("  Exit program.\n");
                exit(EXIT_FAILURE));
        SAFE_REALLOC(mesh->nameout,strlen(mesh->nameout)+6,char,"output mesh name");
        strcat(mesh->nameout,".mesh");
      }
    }
    else if ( !strstr(mesh->nameout,".mesh") ) {
      ADD_MEM(mesh,6*sizeof(char),"input file name",
              printf("  Exit program.\n");
              exit(EXIT_FAILURE));
      SAFE_REALLOC(mesh->nameout,strlen(mesh->nameout)+7,char,"input file name");
      strcat(mesh->nameout,".meshb");
    }
  }
  else if( !(inm = GmfOpenMesh(data,GmfRead,&mesh->ver,&mesh->dim)) ) {
    fprintf(stderr,"  ** %s  NOT FOUND.\n",data);
    return(0);
  }
  fprintf(stdout,"  %%%% %s OPENED\n",data);

  mesh->npi = GmfStatKwd(inm,GmfVertices);
  mesh->nti = GmfStatKwd(inm,GmfTriangles);
  mesh->nei = GmfStatKwd(inm,GmfTetrahedra);
  mesh->nai = GmfStatKwd(inm,GmfEdges);
  if ( !mesh->npi || !mesh->nei ) {
    fprintf(stdout,"  ** MISSING DATA.\n");
    fprintf(stdout," Check that your mesh contains points and tetrahedra.\n");
    fprintf(stdout," Exit program.\n");
    return(0);
  }
  /* memory allocation */
  mesh->np = mesh->npi;
  mesh->nt = mesh->nti;
  mesh->ne = mesh->nei;
  mesh->na = mesh->nai;
  if ( !zaldy(mesh) )  return(0);

  /* read mesh vertices */
  GmfGotoKwd(inm,GmfVertices);
  for (k=1; k<=mesh->np; k++) {
    ppt = &mesh->point[k];
    if (mesh->ver == GmfFloat) {
      GmfGetLin(inm,GmfVertices,&fp1,&fp2,&fp3,&ppt->ref);
      ppt->c[0] = fp1;
      ppt->c[1] = fp2;
      ppt->c[2] = fp3;
    } else {
      GmfGetLin(inm,GmfVertices,&ppt->c[0],&ppt->c[1],&ppt->c[2],&ppt->ref);
    }
    ppt->tag  = MG_NUL;
    ppt->tmp  = 0;
  }

  /* get required vertices */
  np = GmfStatKwd(inm,GmfRequiredVertices);
  if ( np ) {
    GmfGotoKwd(inm,GmfRequiredVertices);
    for (k=1; k<=np; k++) {
      GmfGetLin(inm,GmfRequiredVertices,&i);
      assert(i <= mesh->np);
      ppt = &mesh->point[i];
      ppt->tag |= MG_REQ;
    }
  }
  /* get corners */
  nc = GmfStatKwd(inm,GmfCorners);
  if ( nc ) {
    GmfGotoKwd(inm,GmfCorners);
    for (k=1; k<=nc; k++) {
      GmfGetLin(inm,GmfCorners,&i);
      assert(i <= mesh->np);
      ppt = &mesh->point[i];
      ppt->tag |= MG_CRN;
    }
  }

  /* read mesh triangles */
  nt = 0;
  if ( mesh->nt ) {
    /* Skip triangles with MG_ISO refs */
    if( mesh->info.iso ) {
      GmfGotoKwd(inm,GmfTriangles);
      nt = mesh->nt;
      mesh->nt = 0;
      SAFE_CALLOC(ina,nt+1,int);

      for (k=1; k<=nt; k++) {
        GmfGetLin(inm,GmfTriangles,&v[0],&v[1],&v[2],&ref);
        if( abs(ref) != MG_ISO ) {
          pt1 = &mesh->tria[++mesh->nt];
          pt1->v[0] = v[0];
          pt1->v[1] = v[1];
          pt1->v[2] = v[2];
          pt1->ref = ref;
          ina[k]=mesh->nt;
        }
      }
      if( !mesh->nt )
        DEL_MEM(mesh,mesh->tria,(mesh->nt+1)*sizeof(Tria));

      else if ( mesh->nt < nt ) {
        ADD_MEM(mesh,(mesh->nt+1)*sizeof(Tria),"triangles",
                printf("  Exit program.\n");
                exit(EXIT_FAILURE));
        SAFE_RECALLOC(mesh->tria,nt+1,(mesh->nt+1),Tria,"triangles");
      }
    }
    else {
      GmfGotoKwd(inm,GmfTriangles);
      for (k=1; k<=mesh->nt; k++) {
        pt1 = &mesh->tria[k];
        GmfGetLin(inm,GmfTriangles,&pt1->v[0],&pt1->v[1],&pt1->v[2],&pt1->ref);
      }
    }

    /* get required triangles */
    nt = GmfStatKwd(inm,GmfRequiredTriangles);
    if ( nt ) {
      GmfGotoKwd(inm,GmfRequiredTriangles);
      for (k=1; k<=nt; k++) {
        GmfGetLin(inm,GmfRequiredTriangles,&i);
        assert(i <= mesh->nt);
        if( mesh->info.iso ){
          if( ina[i] == 0 ) continue;
          else {
            pt1 = &mesh->tria[ina[i]];
            pt1->tag[0] |= MG_REQ;
            pt1->tag[1] |= MG_REQ;
            pt1->tag[2] |= MG_REQ;
          }
        }
        else{
          pt1 = &mesh->tria[i];
          pt1->tag[0] |= MG_REQ;
          pt1->tag[1] |= MG_REQ;
          pt1->tag[2] |= MG_REQ;
        }

      }
    }
    if ( mesh->info.iso )
      SAFE_FREE(ina);
  }

  /* read mesh edges */
  nr = nre = 0;
  if ( mesh->na ) {
    na = mesh->na;
    if (mesh->info.iso ) {
      mesh->na = 0;
      SAFE_CALLOC(ina,na+1,int);
    }

    GmfGotoKwd(inm,GmfEdges);

    for (k=1; k<=na; k++) {
      pa = &mesh->edge[k];
      GmfGetLin(inm,GmfEdges,&pa->a,&pa->b,&pa->ref);
      pa->tag |= MG_REF;
      if ( mesh->info.iso ) {
        if( abs(pa->ref) != MG_ISO ) {
          ++mesh->na;
          pa->ref = abs(pa->ref);
          memmove(&mesh->edge[mesh->na],&mesh->edge[k],sizeof(Edge));
          ina[k] = mesh->na;
        }
      }
    }

    /* get ridges */
    nr = GmfStatKwd(inm,GmfRidges);
    if ( nr ) {
      GmfGotoKwd(inm,GmfRidges);
      for (k=1; k<=nr; k++) {
        GmfGetLin(inm,GmfRidges,&ia);
        assert(ia <= na);
        if( mesh->info.iso ){
          if( ina[ia] == 0 )
						continue;
          else {
            pa = &mesh->edge[ina[ia]];
            pa->tag |= MG_GEO;
          }
        }
        else{
          pa = &mesh->edge[ia];
          pa->tag |= MG_GEO;
        }
      }
    }
    /* get required edges */
    nre = GmfStatKwd(inm,GmfRequiredEdges);
    if ( nre ) {
      GmfGotoKwd(inm,GmfRequiredEdges);
      for (k=1; k<=nre; k++) {
        GmfGetLin(inm,GmfRequiredEdges,&ia);
        assert(ia <= na);
        if( mesh->info.iso ){
          if( ina[ia] == 0 ) continue;
          else {
            pa = &mesh->edge[ina[ia]];
            pa->tag |= MG_REQ;
          }
        }
        else{
          pa = &mesh->edge[ia];
          pa->tag |= MG_REQ;
        }

      }
    }
    if (mesh->info.iso )
      SAFE_FREE(ina);
  }

  /* read mesh tetrahedra */
  GmfGotoKwd(inm,GmfTetrahedra);
  for (k=1; k<=mesh->ne; k++) {
    pt = &mesh->tetra[k];
    GmfGetLin(inm,GmfTetrahedra,&pt->v[0],&pt->v[1],&pt->v[2],&pt->v[3],&pt->ref);
    pt->qual = orcal(mesh,k);
    for (i=0; i<4; i++) {
      ppt = &mesh->point[pt->v[i]];
      ppt->tag &= ~MG_NUL;
    }

    if ( mesh->info.iso )  pt->ref = 0;

    /* Possibly switch 2 vertices number so that each tet is positively oriented */
    if ( orvol(mesh->point,pt->v) < 0.0 ) {
      /* mesh->xt temporary used to count reoriented tetra */
      mesh->xt++;
      aux = pt->v[2];
      pt->v[2] = pt->v[3];
      pt->v[3] = aux;
    }
  }
  /* get required tetrahedra */
  nereq = GmfStatKwd(inm,GmfRequiredTetrahedra);
  if ( nereq ) {
    GmfGotoKwd(inm,GmfRequiredTetrahedra);
    for (k=1; k<=nereq; k++) {
      GmfGetLin(inm,GmfRequiredTetrahedra,&i);
      assert(i <= mesh->ne);
      pt = &mesh->tetra[i];
      pt->tag |= MG_REQ;
    }
  }


  /* stats */
  if ( abs(mesh->info.imprim) > 3 ) {
    fprintf(stdout,"     NUMBER OF VERTICES     %8d\n",mesh->np);
    if ( mesh->na ) {
      fprintf(stdout,"     NUMBER OF EDGES        %8d\n",mesh->na);
      if ( nr )
        fprintf(stdout,"     NUMBER OF RIDGES       %8d\n",nr);
        }
    if ( mesh->nt )
      fprintf(stdout,"     NUMBER OF TRIANGLES    %8d\n",mesh->nt);
    fprintf(stdout,"     NUMBER OF ELEMENTS     %8d\n",mesh->ne);

    if ( np || nre || nt || nereq ) {
      fprintf(stdout,"     NUMBER OF REQUIRED ENTITIES: \n");
      if ( np )
        fprintf(stdout,"                  VERTICES    %8d \n",np);
      if ( nre )
        fprintf(stdout,"                  EDGES       %8d \n",nre);
      if ( nt )
        fprintf(stdout,"                  TRIANGLES   %8d \n",nt);
      if ( nereq )
        fprintf(stdout,"                  TETRAHEDRAS %8d \n",nereq);
    }
    if(nc) fprintf(stdout,"     NUMBER OF CORNERS        %8d \n",nc);
  }
  GmfCloseMesh(inm);
  return(1);
}

/** Save mesh data */
int saveMesh(pMesh mesh) {
  pPoint       ppt;
  pTetra       pt;
  pTria        ptt;
  xPoint      *pxp;
  hgeom       *ph;
  int          k,i,na,nc,np,ne,nn,nr,nre,nedreq,ntreq,nt,outm,nereq;
  char         data[128];

  mesh->ver = GmfDouble;
  strcpy(data,mesh->nameout);
  if ( !(outm = GmfOpenMesh(data,GmfWrite,mesh->ver,mesh->dim)) ) {
    fprintf(stderr,"  ** UNABLE TO OPEN %s\n",data);
    return(0);
  }
  fprintf(stdout,"  %%%% %s OPENED\n",data);

  /* vertices */
  np = nc = na = nr = nre = 0;
  for (k=1; k<=mesh->np; k++) {
    ppt = &mesh->point[k];
    if ( MG_VOK(ppt) ) {
      ppt->tmp = ++np;
      if ( ppt->tag & MG_CRN )  nc++;
      if ( ppt->tag & MG_REQ )  nre++;
    }
  }
  GmfSetKwd(outm,GmfVertices,np);
  for (k=1; k<=mesh->np; k++) {
    ppt = &mesh->point[k];
    if ( MG_VOK(ppt) )
      GmfSetLin(outm,GmfVertices,ppt->c[0],ppt->c[1],ppt->c[2],abs(ppt->ref));
  }

  /* corners+required */
  if ( nc ) {
    GmfSetKwd(outm,GmfCorners,nc);
    for (k=1; k<=mesh->np; k++) {
      ppt = &mesh->point[k];
      if ( MG_VOK(ppt) && ppt->tag & MG_CRN )
        GmfSetLin(outm,GmfCorners,ppt->tmp);
    }
  }
  if ( nre ) {
    GmfSetKwd(outm,GmfRequiredVertices,nre);
    for (k=1; k<=mesh->np; k++) {
      ppt = &mesh->point[k];
      if ( MG_VOK(ppt) && ppt->tag & MG_REQ )
        GmfSetLin(outm,GmfRequiredVertices,ppt->tmp);
    }
  }

  nn = nt = 0;
  if ( mesh->xp ) {
    /* Count tangents and normals */
    for (k=1; k<=mesh->np; k++) {
      ppt = &mesh->point[k];
      if ( !MG_VOK(ppt) || MG_SIN(ppt->tag) )  continue;
      else if ( (ppt->tag & MG_BDY)
                && (!(ppt->tag & MG_GEO) || (ppt->tag & MG_NOM)) )
        nn++;
      if ( MG_EDG(ppt->tag) || (ppt->tag & MG_NOM) ) nt++;
    }

    /* write normals */
    GmfSetKwd(outm,GmfNormals,nn);

    for (k=1; k<=mesh->np; k++) {
      ppt = &mesh->point[k];
      if ( !MG_VOK(ppt) || MG_SIN(ppt->tag) )  continue;
      else if ( (ppt->tag & MG_BDY)
                && (!(ppt->tag & MG_GEO) || (ppt->tag & MG_NOM)) ) {
        pxp = &mesh->xpoint[ppt->xp];
        GmfSetLin(outm,GmfNormals,pxp->n1[0],pxp->n1[1],pxp->n1[2]);
      }
    }

    GmfSetKwd(outm,GmfNormalAtVertices,nn);
    nn = 0;
    for (k=1; k<=mesh->np; k++) {
      ppt = &mesh->point[k];
      if ( !MG_VOK(ppt) || MG_SIN(ppt->tag) )  continue;
      else if ( (ppt->tag & MG_BDY)
                && (!(ppt->tag & MG_GEO) || (ppt->tag & MG_NOM)) )
        GmfSetLin(outm,GmfNormalAtVertices,ppt->tmp,++nn);
    }

    if ( nt ) {
      /* Write tangents */
      GmfSetKwd(outm,GmfTangents,nt);
      for (k=1; k<=mesh->np; k++) {
        ppt = &mesh->point[k];
        if ( !MG_VOK(ppt) || MG_SIN(ppt->tag) )  continue;
        else if ( MG_EDG(ppt->tag) || (ppt->tag & MG_NOM) ) {
          pxp = &mesh->xpoint[ppt->xp];
          GmfSetLin(outm,GmfTangents,pxp->t[0],pxp->t[1],pxp->t[2]);
        }
      }
      GmfSetKwd(outm,GmfTangentAtVertices,nt);
      nt = 0;
      for (k=1; k<=mesh->np; k++) {
        ppt = &mesh->point[k];
        if ( !MG_VOK(ppt) || MG_SIN(ppt->tag) )  continue;
        else if ( MG_EDG(ppt->tag) || (ppt->tag & MG_NOM) )
          GmfSetLin(outm,GmfTangentAtVertices,ppt->tmp,++nt);
      }
    }
  }
  DEL_MEM(mesh,mesh->xpoint,(mesh->xpmax+1)*sizeof(xPoint));
  mesh->xp = 0;

  /* boundary mesh */
  /* tria + required tria */
  mesh->nt = ntreq = 0;
  if ( mesh->tria )
    DEL_MEM(mesh,mesh->tria,(mesh->nt+1)*sizeof(Tria));

  chkNumberOfTri(mesh);
  if ( bdryTria(mesh) ) {
    GmfSetKwd(outm,GmfTriangles,mesh->nt);
    for (k=1; k<=mesh->nt; k++) {
      ptt = &mesh->tria[k];
      if ( ptt->tag[0] & MG_REQ && ptt->tag[1] & MG_REQ && ptt->tag[2] & MG_REQ )  ntreq++;
      GmfSetLin(outm,GmfTriangles,mesh->point[ptt->v[0]].tmp,mesh->point[ptt->v[1]].tmp, \
                mesh->point[ptt->v[2]].tmp,ptt->ref);
    }
    if ( ntreq ) {
      GmfSetKwd(outm,GmfRequiredTriangles,ntreq);
      for (k=0; k<=mesh->nt; k++) {
        ptt = &mesh->tria[k];
        if ( ptt->tag[0] & MG_REQ && ptt->tag[1] & MG_REQ && ptt->tag[2] & MG_REQ )
          GmfSetLin(outm,GmfRequiredTriangles,k);
      }
    }
    /* Release memory before htab allocation */
    if ( mesh->adjt )
      DEL_MEM(mesh,mesh->adjt,(3*mesh->nt+4)*sizeof(int));
    if ( mesh->adja )
      DEL_MEM(mesh,mesh->adja,(4*mesh->nemax+5)*sizeof(int));

    /* build hash table for edges */
    if ( mesh->htab.geom )
      DEL_MEM(mesh,mesh->htab.geom,(mesh->htab.max+1)*sizeof(hgeom));

    /* in the wost case (all edges are marked), we will have around 1 edge per *
     * triangle (we count edges only one time) */
    na = nr = nedreq = 0;
    mesh->memCur += (long long)((3*mesh->nt+2)*sizeof(hgeom));
    if ( (mesh->memCur) > (mesh->memMax) ) {
      mesh->memCur -= (long long)((3*mesh->nt+2)*sizeof(hgeom));
      fprintf(stdout,"  ## Warning:");
      fprintf(stdout," unable to allocate htab.\n");
    }
    else if ( hNew(&mesh->htab,mesh->nt,3*(mesh->nt),0) ) {
      for (k=1; k<=mesh->ne; k++) {
        pt   = &mesh->tetra[k];
        if ( MG_EOK(pt) &&  pt->xt ) {
          for (i=0; i<6; i++) {
            if ( mesh->xtetra[pt->xt].edg[i] ||
                 ( MG_EDG(mesh->xtetra[pt->xt].tag[i] ) ||
                   (mesh->xtetra[pt->xt].tag[i] & MG_REQ) ) )
              hEdge(mesh,pt->v[iare[i][0]],pt->v[iare[i][1]],
                    mesh->xtetra[pt->xt].edg[i],mesh->xtetra[pt->xt].tag[i]);
          }
        }
      }
      /* edges + ridges + required edges */
      for (k=0; k<=mesh->htab.max; k++) {
        ph = &mesh->htab.geom[k];
        if ( !ph->a )  continue;
        na++;
        if ( ph->tag & MG_GEO )  nr++;
        if ( ph->tag & MG_REQ )  nedreq++;
      }
      if ( na ) {
        GmfSetKwd(outm,GmfEdges,na);
        for (k=0; k<=mesh->htab.max; k++) {
          ph = &mesh->htab.geom[k];
          if ( !ph->a )  continue;
          GmfSetLin(outm,GmfEdges,mesh->point[ph->a].tmp,mesh->point[ph->b].tmp,ph->ref);
        }
        if ( nr ) {
          GmfSetKwd(outm,GmfRidges,nr);
          na = 0;
          for (k=0; k<=mesh->htab.max; k++) {
            ph = &mesh->htab.geom[k];
            if ( !ph->a )  continue;
            na++;
            if ( ph->tag & MG_GEO )  GmfSetLin(outm,GmfRidges,na);
          }
        }
        if ( nedreq ) {
          GmfSetKwd(outm,GmfRequiredEdges,nedreq);
          na = 0;
          for (k=0; k<=mesh->htab.max; k++) {
            ph = &mesh->htab.geom[k];
            if ( !ph->a )  continue;
            na++;
            if ( ph->tag & MG_REQ )  GmfSetLin(outm,GmfRequiredEdges,na);
          }
        }
      }
      //freeXTets(mesh);
      DEL_MEM(mesh,mesh->htab.geom,(mesh->htab.max+1)*sizeof(hgeom));
    }
    else
      mesh->memCur -= (long long)((3*mesh->nt+2)*sizeof(hgeom));
  }

  /* tetrahedra */
  ne = nereq = 0;
  for (k=1; k<=mesh->ne; k++) {
    pt = &mesh->tetra[k];
    if ( !MG_EOK(pt) ) continue;
    ne++;
    if ( pt->tag & MG_REQ ){
      nereq++;
    }
  }

  GmfSetKwd(outm,GmfTetrahedra,ne);
  for (k=1; k<=mesh->ne; k++) {
    pt = &mesh->tetra[k];
    if ( MG_EOK(pt) ) GmfSetLin(outm,GmfTetrahedra,mesh->point[pt->v[0]].tmp,mesh->point[pt->v[1]].tmp, \
                                mesh->point[pt->v[2]].tmp,mesh->point[pt->v[3]].tmp,pt->ref);
  }

  if ( nereq ) {
    GmfSetKwd(outm,GmfRequiredTetrahedra,nereq);
    ne = 0;
    for (k=1; k<=mesh->ne; k++) {
      pt = &mesh->tetra[k];
      if ( !MG_EOK(pt) ) continue;
      ne++;
      if ( pt->tag & MG_REQ ) GmfSetLin(outm,GmfRequiredTetrahedra,ne);
    }
  }

  if ( mesh->info.imprim ) {
    fprintf(stdout,"     NUMBER OF VERTICES   %8d   CORNERS %8d\n",np,nc+nre);
    if ( na )
      fprintf(stdout,"     NUMBER OF EDGES      %8d   RIDGES  %8d\n",na,nr);
    if ( mesh->nt )
      fprintf(stdout,"     NUMBER OF TRIANGLES  %8d\n",mesh->nt);
    fprintf(stdout,"     NUMBER OF ELEMENTS   %8d\n",mesh->ne);
  }

  GmfCloseMesh(outm);
  return(1);
}

/** Save mesh data without adja and xtetra tables (for library version) */
int saveLibraryMesh(pMesh mesh) {
  pPoint       ppt;
  pTetra       pt;
  pTria        ptt;
  int          k,nc,np,ne,nr,nre,nedreq,ntreq,outm,nereq;
  char         data[128];

  mesh->ver = GmfDouble;
  strcpy(data,mesh->nameout);
  if ( !(outm = GmfOpenMesh(data,GmfWrite,mesh->ver,mesh->dim)) ) {
    fprintf(stderr,"  ** UNABLE TO OPEN %s\n",data);
    return(0);
  }
  fprintf(stdout,"  %%%% %s OPENED\n",data);

  /* vertices */
  np = nc = nre = 0;
  for (k=1; k<=mesh->np; k++) {
    ppt = &mesh->point[k];
    if ( MG_VOK(ppt) ) {
      ppt->tmp = ++np;
      if ( ppt->tag & MG_CRN )  nc++;
      if ( ppt->tag & MG_REQ )  nre++;
    }
  }
  GmfSetKwd(outm,GmfVertices,np);
  for (k=1; k<=mesh->np; k++) {
    ppt = &mesh->point[k];
    if ( MG_VOK(ppt) )
      GmfSetLin(outm,GmfVertices,ppt->c[0],ppt->c[1],ppt->c[2],ppt->ref);
  }

  /* corners+required */
  if ( nc ) {
    GmfSetKwd(outm,GmfCorners,nc);
    for (k=1; k<=mesh->np; k++) {
      ppt = &mesh->point[k];
      if ( MG_VOK(ppt) && ppt->tag & MG_CRN )
        GmfSetLin(outm,GmfCorners,ppt->tmp);
    }
  }
  if ( nre ) {
    GmfSetKwd(outm,GmfRequiredVertices,nre);
    for (k=1; k<=mesh->np; k++) {
      ppt = &mesh->point[k];
      if ( MG_VOK(ppt) && ppt->tag & MG_REQ )
        GmfSetLin(outm,GmfRequiredVertices,ppt->tmp);
    }
  }

  /* boundary mesh */
  /* tria + required tria */
  ntreq = 0;
  if ( mesh->nt ) {
    GmfSetKwd(outm,GmfTriangles,mesh->nt);
    for (k=1; k<=mesh->nt; k++) {
      ptt = &mesh->tria[k];
      if ( ptt->tag[0] & MG_REQ && ptt->tag[1] & MG_REQ && ptt->tag[2] & MG_REQ )  ntreq++;
      GmfSetLin(outm,GmfTriangles,mesh->point[ptt->v[0]].tmp,mesh->point[ptt->v[1]].tmp, \
                mesh->point[ptt->v[2]].tmp,ptt->ref);
    }
    if ( ntreq ) {
      GmfSetKwd(outm,GmfRequiredTriangles,ntreq);
      for (k=0; k<=mesh->nt; k++) {
        ptt = &mesh->tria[k];
        if ( ptt->tag[0] & MG_REQ && ptt->tag[1] & MG_REQ && ptt->tag[2] & MG_REQ )
          GmfSetLin(outm,GmfRequiredTriangles,k);
      }
    }
  }

  nr = nedreq = 0;
  if ( mesh->na ) {
    GmfSetKwd(outm,GmfEdges,mesh->na);
    for (k=1; k<=mesh->na; k++) {
      GmfSetLin(outm,GmfEdges,mesh->point[mesh->edge[k].a].tmp,
                mesh->point[mesh->edge[k].b].tmp,mesh->edge[k].ref);
      if ( mesh->edge[k].tag & MG_GEO ) nr++;
      if ( mesh->edge[k].tag & MG_REQ ) nedreq++;
    }

    if ( nr ) {
      GmfSetKwd(outm,GmfRidges,nr);
      for (k=1; k<=mesh->na; k++) {
        if ( mesh->edge[k].tag & MG_GEO )  GmfSetLin(outm,GmfRidges,k);
      }
    }
    if ( nedreq ) {
      GmfSetKwd(outm,GmfRequiredEdges,nedreq);
      for (k=1; k<=mesh->na; k++) {
        if (  mesh->edge[k].tag & MG_REQ )  GmfSetLin(outm,GmfRequiredEdges,k);
      }
    }
  }

  /* tetrahedra */
  ne = nereq = 0;
  for (k=1; k<=mesh->ne; k++) {
    pt = &mesh->tetra[k];
    if ( !MG_EOK(pt) ) continue;
    ne++;
    if ( pt->tag & MG_REQ ){
      nereq++;
    }
  }

  GmfSetKwd(outm,GmfTetrahedra,ne);
  for (k=1; k<=mesh->ne; k++) {
    pt = &mesh->tetra[k];
    if ( MG_EOK(pt) ) GmfSetLin(outm,GmfTetrahedra,mesh->point[pt->v[0]].tmp,mesh->point[pt->v[1]].tmp, \
                                mesh->point[pt->v[2]].tmp,mesh->point[pt->v[3]].tmp,pt->ref);
  }

  if ( nereq ) {
    GmfSetKwd(outm,GmfRequiredTetrahedra,nereq);
    ne = 0;
    for (k=1; k<=mesh->ne; k++) {
      pt = &mesh->tetra[k];
      if ( !MG_EOK(pt) ) continue;
      ne++;
      if ( pt->tag & MG_REQ ) GmfSetLin(outm,GmfRequiredTetrahedra,ne);
    }
  }

  if ( mesh->info.imprim ) {
    fprintf(stdout,"     NUMBER OF VERTICES   %8d   CORNERS %8d\n",np,nc+nre);
    if ( mesh->na )
      fprintf(stdout,"     NUMBER OF EDGES      %8d   RIDGES  %8d\n",mesh->na,nr);
    if ( mesh->nt )
      fprintf(stdout,"     NUMBER OF TRIANGLES  %8d\n",mesh->nt);
    fprintf(stdout,"     NUMBER OF ELEMENTS   %8d\n",mesh->ne);
  }

  GmfCloseMesh(outm);
  return(1);
}

/** load metric field */
int loadMet(pMesh mesh,pSol met) {
  double       dbuf[GmfMaxTyp];
  float        fbuf[GmfMaxTyp];
  int          k,inm,typtab[GmfMaxTyp];
  char        *ptr,data[128];

  if ( !met->namein )  return(0);
  strcpy(data,met->namein);
  ptr = strstr(data,".sol");
  if ( !ptr ) {
    /* data contains the filename without extension */
    strcat(data,".solb");
    if (!(inm = GmfOpenMesh(data,GmfRead,&met->ver,&met->dim)) ) {
      /* our file is not a .solb file, try with .sol ext */
      ptr  = strstr(data,".sol");
      *ptr = '\0';
      strcat(data,".sol");
      if (!(inm = GmfOpenMesh(data,GmfRead,&met->ver,&met->dim)) ) {
        fprintf(stderr,"  ** %s  NOT FOUND. USE DEFAULT METRIC.\n",data);
        return(-1);
      }
    }
  }
  else {
    if (!(inm = GmfOpenMesh(data,GmfRead,&met->ver,&met->dim)) ) {
      fprintf(stderr,"  ** %s  NOT FOUND. USE DEFAULT METRIC.\n",data);
      return(-1);
    }
  }
  fprintf(stdout,"  %%%% %s OPENED\n",data);

  /* read solution or metric */
  met->np = GmfStatKwd(inm,GmfSolAtVertices,&met->type,&met->size,typtab);
  if ( !met->np ) {
    fprintf(stdout,"  ** MISSING DATA. No solution.\n");
    return(1);
  }
  if ( (met->type != 1) || (typtab[0] != 1 && typtab[0] != 3) ) {
    fprintf(stdout,"  ** DATA IGNORED %d  %d\n",met->type,typtab[0]);
    met->np = met->npmax = 0;
    return(-1);
  }

  met->npi = met->np;

  /* mem alloc */
  if ( met->m )  DEL_MEM(mesh,met->m,(met->size*met->npmax+1)*sizeof(double));
  met->npmax = mesh->npmax;

  ADD_MEM(mesh,(met->size*met->npmax+1)*sizeof(double),"initial solution",
          printf("  Exit program.\n");
          exit(EXIT_FAILURE));
  SAFE_CALLOC(met->m,met->size*met->npmax+1,double);

  /* read mesh solutions */
  GmfGotoKwd(inm,GmfSolAtVertices);
  /* isotropic metric */
  if ( met->size == 1 ) {
    if ( met->ver == GmfFloat ) {
      for (k=1; k<=met->np; k++) {
        GmfGetLin(inm,GmfSolAtVertices,fbuf);
        met->m[k] = fbuf[0];
      }
    }
    else {
      for (k=1; k<=met->np; k++) {
        GmfGetLin(inm,GmfSolAtVertices,dbuf);
        met->m[k] = dbuf[0];
      }
    }
  }
  /* anisotropic metric */
  /*else {
    if ( met->ver == GmfFloat ) {
    for (k=1; k<=met->np; k++) {
    GmfGetLin(inm,GmfSolAtVertices,fbuf);
    tmpf    = fbuf[2];
    fbuf[2] = fbuf[3];
    fbuf[3] = tmpf;
    for (i=0; i<6; i++)  met->m[6*k+1+i] = fbuf[i];
    }
    }
    else {
    for (k=1; k<=met->np; k++) {
    GmfGetLin(inm,GmfSolAtVertices,dbuf);
    tmpd    = dbuf[2];
    dbuf[2] = dbuf[3];
    dbuf[3] = tmpd;
    for (i=0; i<met->size; i++)  met->m[6*k+1+i] = dbuf[i];
    }
    }
    }*/
  met->npi = met->np;
  GmfCloseMesh(inm);
  return(1);
}

/** write iso or aniso metric */
int saveMet(pMesh mesh,pSol met) {
  pPoint     ppt;
  double     dbuf[GmfMaxTyp]/*,tmp*/;
  int        k/*,i*/,np,outm,nbm,typtab[GmfMaxTyp];
  char      *ptr,data[128];

  if ( !met->m )  return(-1);
  met->ver = GmfDouble;
  strcpy(data,met->nameout);
  ptr = strstr(data,".mesh");
  if ( ptr )  *ptr = '\0';
  ptr = strstr(data,".sol");
  if ( !ptr )  strcat(data,".sol");
  if ( !(outm = GmfOpenMesh(data,GmfWrite,met->ver,met->dim)) ) {
    fprintf(stderr,"  ** UNABLE TO OPEN %s\n",data);
    return(0);
  }
  fprintf(stdout,"  %%%% %s OPENED\n",data);

  np = 0;
  for (k=1; k<=mesh->np; k++) {
    ppt = &mesh->point[k];
    if ( MG_VOK(ppt) )  np++;
  }

  /* write isotropic metric */
  if ( met->size == 1 ) {
    typtab[0] = 1;
    nbm = 1;
    GmfSetKwd(outm,GmfSolAtVertices,np,nbm,typtab);
    for (k=1; k<=mesh->np; k++) {
      ppt = &mesh->point[k];
      if ( MG_VOK(ppt) ) {
        dbuf[0] = met->m[k];
        GmfSetLin(outm,GmfSolAtVertices,dbuf);
      }
    }
  }
  /* write anisotropic metric */
  /*else {
    typtab[0] = 3;
    nbm = 1;
    GmfSetKwd(outm,GmfSolAtVertices,np,nbm,typtab);
    for (k=1; k<=mesh->np; k++) {
      ppt = &mesh->point[k];
      if ( MG_VOK(ppt) ) {
        for (i=0; i<met->size; i++)  dbuf[i] = met->m[met->size*(k)+1+i];
        tmp = dbuf[2];
        dbuf[2] = dbuf[3];
        dbuf[3] = tmp;
        GmfSetLin(outm,GmfSolAtVertices,dbuf);
      }
    }
    }*/
  GmfCloseMesh(outm);
  return(1);
}

#ifdef SINGUL
/** Read singul data. Here we suppose that the file contains the singularities *
 *  (corner, required, ridges....) */
int loadSingul(pMesh mesh,pSingul singul) {
  Mesh         sing_mesh;
  pEdge        pa,pas;
  pPoint       ppt;
  psPoint      ppts;
  float        fp1,fp2,fp3;
  int          i,k,inm,nr,nre,nc,npr,na,ns;
  char         *ptr,data[128],*filein;

  filein = singul->namein;
  strcpy(data,filein);
  ptr = strstr(data,".mesh");
  if ( !ptr ) {
    strcat(data,".meshb");
    if( !(inm = GmfOpenMesh(data,GmfRead,&sing_mesh.ver,&sing_mesh.dim)) ) {
      ptr = strstr(data,".mesh");
      *ptr = '\0';
      strcat(data,".mesh");
      if( !(inm = GmfOpenMesh(data,GmfRead,&sing_mesh.ver,&sing_mesh.dim)) ) {
        fprintf(stderr,"  ** %s  NOT FOUND.\n",data);
        return(0);
      }
    }
  }
  else if( !(inm = GmfOpenMesh(data,GmfRead,&sing_mesh.ver,&sing_mesh.dim)) ) {
    fprintf(stderr,"  ** %s  NOT FOUND.\n",data);
    return(0);
  }
  fprintf(stdout,"  %%%% %s OPENED\n",data);

  sing_mesh.np = GmfStatKwd(inm,GmfVertices);
  sing_mesh.nt = GmfStatKwd(inm,GmfTriangles);
  sing_mesh.na = GmfStatKwd(inm,GmfEdges);
  if ( !sing_mesh.np ) {
    fprintf(stdout,"  ** MISSING DATA.\n");
    fprintf(stdout," Check that your mesh contains points.\n");
    fprintf(stdout," Exit program.\n");
    return(0);
  }

  /* memory allocation */
  ADD_MEM(mesh,(sing_mesh.np+1)*sizeof(Point),"points of singularities mesh",
          printf("  Exit program.\n");
          exit(EXIT_FAILURE));
  SAFE_CALLOC(sing_mesh.point,sing_mesh.np+1,Point);

  if ( sing_mesh.nt ) {
    ADD_MEM(mesh,(sing_mesh.nt+1)*sizeof(Tria),"triangles of singularities mesh",
            printf("  Exit program.\n");
            exit(EXIT_FAILURE));
    SAFE_CALLOC(sing_mesh.tria,sing_mesh.nt+1,Tria);
  }
  if ( sing_mesh.na ) {
    ADD_MEM(mesh,(sing_mesh.na+1)*sizeof(Edge),"edges of singularities mesh",
            printf("  Exit program.\n");
            exit(EXIT_FAILURE));
    SAFE_CALLOC(sing_mesh.edge,sing_mesh.na+1,Edge);
  }

  /* find bounding box */
  for (i=0; i<sing_mesh.dim; i++) {
    singul->min[i] =  1.e30;
    singul->max[i] = -1.e30;
  }

  /* read mesh vertices */
  GmfGotoKwd(inm,GmfVertices);
  for (k=1; k<=sing_mesh.np; k++) {
    ppt = &sing_mesh.point[k];
    if (sing_mesh.ver == GmfFloat) {
      GmfGetLin(inm,GmfVertices,&fp1,&fp2,&fp3,&ppt->ref);
      ppt->c[0] = fp1;
      ppt->c[1] = fp2;
      ppt->c[2] = fp3;
    } else {
      GmfGetLin(inm,GmfVertices,&ppt->c[0],&ppt->c[1],&ppt->c[2],&ppt->ref);
    }
    for (i=0; i<sing_mesh.dim; i++) {
      if ( ppt->c[i] > singul->max[i] )  singul->max[i] = ppt->c[i];
      if ( ppt->c[i] < singul->min[i] )  singul->min[i] = ppt->c[i];
    }
    ppt->tag  = MG_NOTAG;
  }

  /* fill singul */
  /* get required vertices */
  npr = GmfStatKwd(inm,GmfRequiredVertices);
  if ( npr ) {
    GmfGotoKwd(inm,GmfRequiredVertices);
    for (k=1; k<=npr; k++) {
      GmfGetLin(inm,GmfRequiredVertices,&i);
      assert(i <= sing_mesh.np);
      ppt = &sing_mesh.point[i];
      ppt->tag |= MG_REQ;
    }
  }
  /* get corners */
  nc = GmfStatKwd(inm,GmfCorners);
  if ( nc ) {
    GmfGotoKwd(inm,GmfCorners);
    for (k=1; k<=nc; k++) {
      GmfGetLin(inm,GmfCorners,&i);
      assert(i <= sing_mesh.np);
      ppt = &sing_mesh.point[i];
      if ( !MG_SIN(ppt->tag) ){
        npr++;
      }
      ppt->tag |= MG_CRN;
    }
  }

  /* read mesh edges */
  if ( sing_mesh.na ) {
    GmfGotoKwd(inm,GmfEdges);

    for (k=1; k<=sing_mesh.na; k++) {
      pa = &sing_mesh.edge[k];
      GmfGetLin(inm,GmfEdges,&pa->a,&pa->b,&pa->ref);
      pa->tag = MG_NOTAG;
    }
  }

  /* get ridges */
  nr = GmfStatKwd(inm,GmfRidges);
  if ( nr ) {
    GmfGotoKwd(inm,GmfRidges);
    for (k=1; k<=nr; k++) {
      GmfGetLin(inm,GmfRidges,&i);
      assert(i <= sing_mesh.na);
      pa = &sing_mesh.edge[i];
      pa->tag |= MG_GEO;
      ppt = &sing_mesh.point[pa->a];
      if ( !(ppt->tag & MG_GEO) ){
        ppt->tag |= MG_GEO;
        if ( !MG_SIN(ppt->tag) )  npr++;
      }
      ppt = &sing_mesh.point[pa->b];
      if ( !(ppt->tag & MG_GEO) ){
        ppt->tag |= MG_GEO;
        if ( !MG_SIN(ppt->tag) )  npr++;
      }
    }
  }
  /* get required edges */
  nre = GmfStatKwd(inm,GmfRequiredEdges);
  na  = 0;
  if ( nre ) {
    GmfGotoKwd(inm,GmfRequiredEdges);
    for (k=1; k<=nre; k++) {
      GmfGetLin(inm,GmfRequiredEdges,&i);
      assert(i <= sing_mesh.na);
      pa = &sing_mesh.edge[i];
      if ( !(pa->tag & MG_GEO) ) na++;
      pa->tag |= MG_REQ;
      ppt = &sing_mesh.point[pa->a];
      if ( !(ppt->tag & MG_REQ) ){
        ppt->tag |= MG_REQ;
        if ( !MG_SIN(ppt->tag) )  npr++;
      }
      ppt = &sing_mesh.point[pa->b];
      if ( !(ppt->tag & MG_REQ) ){
        ppt->tag |= MG_REQ;
        if ( !MG_SIN(ppt->tag) )  npr++;
      }
    }
  }

  singul->ns = npr;
  ns = 1;
  if ( singul->ns ) {
    ADD_MEM(mesh,(singul->ns+1)*sizeof(sPoint),"vertex singularities",
            printf("  Exit program.\n");
            exit(EXIT_FAILURE));
    SAFE_CALLOC(singul->point,singul->ns+1,sPoint);
    for ( k=1; k<=sing_mesh.np; k++ ) {
      ppt = &sing_mesh.point[k];
      if ( (ppt->tag & MG_REQ) || (ppt->tag & MG_GEO) ) {
        ppts = &singul->point[ns];
        ppts->c[0] = ppt->c[0];
        ppts->c[1] = ppt->c[1];
        ppts->c[2] = ppt->c[2];
        ppts->tag  = ppt->tag | MG_SGL;
        ppt->tmp   = ns;
        ns++;
      }
    }
  }


  singul->na = nr+na;
  na = 1;
  if ( singul->na ) {
    ADD_MEM(mesh,(singul->na+1)*sizeof(Edge),"edge singularities",
            printf("  Exit program.\n");
            exit(EXIT_FAILURE));
    SAFE_CALLOC(singul->edge,singul->na+1,Edge);

    for ( k=1; k<=sing_mesh.na; k++ ) {
      pa = &sing_mesh.edge[k];
      if ( (pa->tag & MG_REQ) || (pa->tag & MG_GEO) ) {
        pas = &singul->edge[na];
        pas->a = sing_mesh.point[pa->a].tmp;
        pas->b = sing_mesh.point[pa->b].tmp;
        pas->tag  = pa->tag | MG_SGL;
        na++;
      }
    }
  }

  /* stats */
  if ( singul->ns )
    fprintf(stdout,"     NUMBER OF REQUIRED VERTICES : %8d \n",singul->ns);
  if ( singul->na )
    fprintf(stdout,"     NUMBER OF REQUIRED EDGES    : %8d \n",singul->na);

  GmfCloseMesh(inm);

  /* memory free */
  DEL_MEM(mesh,sing_mesh.point,(sing_mesh.np+1)*sizeof(Point));
  if ( sing_mesh.na ) {
    DEL_MEM(mesh,sing_mesh.edge,(sing_mesh.na+1)*sizeof(Edge));
  }
  if ( sing_mesh.nt ) {
    DEL_MEM(mesh,sing_mesh.tria,(sing_mesh.nt+1)*sizeof(Tria));
  }
  return(1);
}
#endif