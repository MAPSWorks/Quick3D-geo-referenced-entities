
// Application
#include "CLogManager.h"
#include "CRessourcesManager.h"
#include "C3DScene.h"
#include "CAutoTerrain.h"
#include "CTreeGenerator.h"
#include "CTiledMaterial.h"

#include "CGeneratedField.h"
#include "CSRTMField.h"
#include "CHGTField.h"
#include "CBILField.h"

using namespace Math;

#define LAT_MAX  90.0

//-------------------------------------------------------------------------------------------------

CComponent* CAutoTerrain::instanciator(C3DScene* pScene)
{
	return new CAutoTerrain(pScene);
}

//-------------------------------------------------------------------------------------------------

CAutoTerrain::CAutoTerrain(C3DScene* pScene, CGeoloc gCameraPosition, CHeightField* pHeights, bool bGenerateNow)
: CComponent(pScene)
, m_bGenerateNow(bGenerateNow)
, m_pRoot(NULL)
, m_pHeights(pHeights)
, m_pMaterial(NULL)
, m_iLevels(15)
, m_iTerrainResolution(31)
{
	setName("AutoTerrain");
	setInheritTransform(false);

	m_pMaterial = pScene->getRessourcesManager()->shareMaterial(QSharedPointer<CMaterial>(new CMaterial(m_pScene)));

	if (m_bGenerateNow)
	{
		LOG_DEBUG("CAutoTerrain::CAutoTerrain() : Generating terrain now");

		if (pScene->getViewports().count() > 0 && pScene->getViewports()[0]->getCamera() != NULL)
		{
			buildRoot();

			CRenderContext context(
				QMatrix4x4(),
				QMatrix4x4(),
				QMatrix4x4(),
				QMatrix4x4(),
				Math::CMatrix4(),
				pScene,
				pScene->getViewports()[0]->getCamera()
				);

			buildRecurse(m_pRoot, &context, m_iLevels);
		}
		else
		{
			LOG_WARNING("CAutoTerrain::CAutoTerrain() : Scene has no viewport");
		}
	}
}

//-------------------------------------------------------------------------------------------------

CAutoTerrain::~CAutoTerrain()
{
	if (m_pRoot) delete m_pRoot;

	foreach (CVegetation* pVegetation, m_vVegetation)
	{
		delete pVegetation;
	}
}

//-------------------------------------------------------------------------------------------------

void CAutoTerrain::setTerrainResolution(int value)
{
	m_iTerrainResolution = Angles::clipInt(value, 3, 81);
}

//-------------------------------------------------------------------------------------------------

void CAutoTerrain::loadParameters(CXMLNode xComponent)
{
	CComponent::loadParameters(xComponent);

	m_xParameters = xComponent.getNodeByTagName(ParamName_Parameters);

	CXMLNode xGeneralNode = m_xParameters.getNodeByTagName(ParamName_General);

	m_iLevels = xGeneralNode.m_vAttributes[ParamName_Levels].toInt();
	m_iLevels = Angles::clipInt(m_iLevels, 2, 20);

	m_iTerrainResolution = xGeneralNode.m_vAttributes[ParamName_Resolution].toInt();
	m_iTerrainResolution = Angles::clipInt(m_iTerrainResolution, 3, 81);

	CXMLNode xHeightNode = m_xParameters.getNodeByTagName(ParamName_Height);

	QString sType = xHeightNode.m_vAttributes[ParamName_Type];
	QString sPath = xHeightNode.m_vAttributes[ParamName_Path];

	if (sType.toLower() == "srtm")
	{
		m_pHeights = new CSRTMField(m_xParameters, sPath);
	}
	else if (sType.toLower() == "hgt")
	{
		m_pHeights = new CHGTField(m_xParameters, sPath);
	}
	else if (sType.toLower() == "bil")
	{
		m_pHeights = new CBILField(m_xParameters, sPath);
	}
	else
	{
		m_pHeights = new CGeneratedField(m_xParameters);
	}

    readVegetationParameters();

	CXMLNode xMaterialNode = m_xParameters.getNodeByTagName(ParamName_Material);

	QString sMaterialType = xMaterialNode.m_vAttributes[ParamName_Type];

	if (sMaterialType.toLower() == "bing")
	{
		CTiledMaterial* pTiled = new CTiledMaterial(m_pScene);

		pTiled->setIRFactor(0.2);
		pTiled->setLevels(m_iLevels);

		m_pMaterial = m_pScene->getRessourcesManager()->shareMaterial(QSharedPointer<CMaterial>(pTiled));
	}
	else
	{
		m_pMaterial = m_pScene->getRessourcesManager()->shareMaterial(QSharedPointer<CMaterial>(new CMaterial(m_pScene)));

		m_pMaterial->setIRFactor(0.2);

		m_pMaterial->addDiffuseTexture("Textures/Stone01.jpg");
		m_pMaterial->addDiffuseTexture("Textures/Stone02.jpg");
		m_pMaterial->addDiffuseTexture("Textures/Dirt01.jpg");
		m_pMaterial->addDiffuseTexture("Textures/Dirt02.jpg");
		m_pMaterial->addDiffuseTexture("Textures/Sand01.jpg");
		m_pMaterial->addDiffuseTexture("Textures/Snow01.jpg");
		m_pMaterial->addDiffuseTexture("Textures/Grass01.jpg");
		m_pMaterial->addDiffuseTexture("Textures/Grass02.jpg");
	}
}

//-------------------------------------------------------------------------------------------------

void CAutoTerrain::paint(CRenderContext* pContext)
{
	static int iBuildCounter = 0;
	static int iGarbageCounter = 0;

	buildRoot();

	iBuildCounter++;

	if (iBuildCounter > 5)
	{
		iBuildCounter = 0;
		buildRecurse(m_pRoot, pContext, m_iLevels);
	}

	QVector<CWorldChunk*> vChunkCollect;

	paintRecurse(vChunkCollect, pContext, m_pRoot, m_iLevels, false);

	qSort(vChunkCollect.begin(), vChunkCollect.end());

	if (pContext->scene()->isRenderingShadows() == false)
	{
		foreach (CWorldChunk* pChunk, vChunkCollect)
		{
			pContext->m_iNumChunksDrawn++;

			pChunk->paint(pContext, ttGround);
		}

		foreach (CWorldChunk* pChunk, vChunkCollect)
		{
			pChunk->paint(pContext, ttWater);
		}
	}

	foreach (CWorldChunk* pChunk, vChunkCollect)
	{
		pChunk->paint(pContext, ttVegetation);
	}

	// Garbage collection

	iGarbageCounter++;

	if (iGarbageCounter > 10)
	{
		iGarbageCounter = 0;
		collectGarbage();
	}
}

//-------------------------------------------------------------------------------------------------

void CAutoTerrain::update(double dDeltaTime)
{
}

//-------------------------------------------------------------------------------------------------

void CAutoTerrain::postUpdate(double dDeltaTime)
{
}

//-------------------------------------------------------------------------------------------------

void CAutoTerrain::buildRoot()
{
	if (m_pRoot == NULL)
	{
		m_pRoot = new CWorldChunk(m_pScene, this, this);
		m_pRoot->setInheritTransform(false);
		m_pRoot->CComponent::setParent(this);
		m_pRoot->setGeoloc(CGeoloc(0.0, 180.0, 0.0));
		m_pRoot->computeWorldTransform();
		m_pRoot->setSize(CGeoloc(LAT_MAX * 2.0, 360.0, 0.0));
	}
}

//-------------------------------------------------------------------------------------------------

void CAutoTerrain::buildRecurse(CWorldChunk* pChunk, CRenderContext* pContext, int iLevel)
{
	CGeoloc gOriginalChunkPosition = pChunk->getOriginalGeoloc();
	CGeoloc gOriginalChunkSize = pChunk->getOriginalSize();

	CGeoloc gChunkPosition = pChunk->getGeoloc();
	CGeoloc gChunkSize = pChunk->getSize();

	CBoundingBox bChunkBounds = pChunk->getBuildWorldBounds();

	// On d�cide si ce niveau de d�tail est suffisant
	bool bStayHere = (
		bChunkBounds.containsSpherical(pContext->camera()->getGeoloc()) == false ||
		iLevel == 0
		)
		&& iLevel < m_iLevels;

	if (bStayHere)
	{
		if (pChunk->getTerrain() == NULL)
		{
			LOG_DEBUG(QString("Creating terrain for tile at lat %1, lon %2, level %3")
				.arg(gChunkPosition.Latitude)
				.arg(gChunkPosition.Longitude)
				.arg(iLevel)
				);

			CTerrain* pTerrain = new CTerrain(
				m_pScene,
				m_pHeights,
				gOriginalChunkPosition,
				gOriginalChunkSize,
				gChunkPosition,
				gChunkSize,
				m_iTerrainResolution,
				iLevel,
				m_iLevels,
				false,
				m_bGenerateNow
				);

			pTerrain->setMaterial(m_pMaterial);
			pTerrain->setInheritTransform(false);
			pTerrain->setGeoloc(gChunkPosition);
			pTerrain->computeWorldTransform();
			pChunk->setTerrain(pTerrain, m_bGenerateNow);

			if (m_bGenerateNow == false)
			{
				CTiledMaterial* pTiled = dynamic_cast<CTiledMaterial*>(m_pMaterial.data());

				if (pTiled == NULL)
				{
					CTerrain* pWater = new CTerrain(
						m_pScene,
						m_pHeights,
						gOriginalChunkPosition,
						gOriginalChunkSize,
						gChunkPosition,
						gChunkSize,
						(int) ((double) m_iTerrainResolution * 0.75),
						iLevel,
						m_iLevels,
						true,
						m_bGenerateNow
						);

					pWater->setInheritTransform(false);
					pWater->setGeoloc(gChunkPosition);
					pWater->computeWorldTransform();
					pChunk->setWater(pWater);
				}
			}

			pChunk->setLastUsed(QDateTime::currentDateTime());
		}
	}
	else
	{
		// Cr�ation des sous-chunks si besoin
		if (pChunk->getChildren().count() == 0)
		{
			LOG_DEBUG(QString("Creating sub-quads for tile at lat %1, lon %2, level %3")
				.arg(gChunkPosition.Latitude)
				.arg(gChunkPosition.Longitude)
				.arg(iLevel)
				);

			CGeoloc gStart(
				gOriginalChunkPosition.Latitude - gOriginalChunkSize.Latitude * 0.5,
				gOriginalChunkPosition.Longitude - gOriginalChunkSize.Longitude * 0.5,
				0.0
				);

			CGeoloc gSize(
				gOriginalChunkSize.Latitude * 0.5,
				gOriginalChunkSize.Longitude * 0.5,
				0.0
				);

			CWorldChunk* pChild1 = new CWorldChunk(m_pScene, this, this);
			CWorldChunk* pChild2 = new CWorldChunk(m_pScene, this, this);
			CWorldChunk* pChild3 = new CWorldChunk(m_pScene, this, this);
			CWorldChunk* pChild4 = new CWorldChunk(m_pScene, this, this);

			pChild1->setInheritTransform(false);
			pChild2->setInheritTransform(false);
			pChild3->setInheritTransform(false);
			pChild4->setInheritTransform(false);

			pChild1->CComponent::setParent((CComponent*) pChunk);
			pChild2->CComponent::setParent((CComponent*) pChunk);
			pChild3->CComponent::setParent((CComponent*) pChunk);
			pChild4->CComponent::setParent((CComponent*) pChunk);

			pChunk->getChildren().append(pChild1);
			pChunk->getChildren().append(pChild2);
			pChunk->getChildren().append(pChild3);
			pChunk->getChildren().append(pChild4);

			pChild1->setGeoloc(CGeoloc(gStart.Latitude + gOriginalChunkSize.Latitude * 0.25, gStart.Longitude + gOriginalChunkSize.Longitude * 0.25, 0.0));
			pChild2->setGeoloc(CGeoloc(gStart.Latitude + gOriginalChunkSize.Latitude * 0.25, gStart.Longitude + gOriginalChunkSize.Longitude * 0.75, 0.0));
			pChild3->setGeoloc(CGeoloc(gStart.Latitude + gOriginalChunkSize.Latitude * 0.75, gStart.Longitude + gOriginalChunkSize.Longitude * 0.25, 0.0));
			pChild4->setGeoloc(CGeoloc(gStart.Latitude + gOriginalChunkSize.Latitude * 0.75, gStart.Longitude + gOriginalChunkSize.Longitude * 0.75, 0.0));

			pChild1->computeWorldTransform();
			pChild2->computeWorldTransform();
			pChild3->computeWorldTransform();
			pChild4->computeWorldTransform();

			pChild1->setSize(gSize);
			pChild2->setSize(gSize);
			pChild3->setSize(gSize);
			pChild4->setSize(gSize);

			pChild1->build();
			pChild2->build();
			pChild3->build();
			pChild4->build();
		}

		foreach (CComponent* pChildComponent, pChunk->getChildren())
		{
			CWorldChunk* pChild = dynamic_cast<CWorldChunk*>(pChildComponent);

			buildRecurse(pChild, pContext, iLevel - 1);
		}
	}
}

//-------------------------------------------------------------------------------------------------

void CAutoTerrain::paintRecurse(QVector<CWorldChunk*>& vChunkCollect, CRenderContext* pContext, CWorldChunk* pChunk, int iLevel, bool bForcePaint)
{
	CGeoloc gChunkPosition = pChunk->getGeoloc();
	CGeoloc gChunkSize = pChunk->getSize();
	CBoundingBox bChunkBounds = pChunk->getWorldBounds();

	// On d�cide si ce niveau de d�tail est suffisant
	bool bStayHere = (
		bChunkBounds.containsSpherical(pContext->camera()->getGeoloc()) == false ||
		iLevel == 0
		)
		&& iLevel < m_iLevels;

	bool bChildrenDrawable = true;

	foreach (CComponent* pChildComponent, pChunk->getChildren())
	{
		CWorldChunk* pChild = dynamic_cast<CWorldChunk*>(pChildComponent);

		if (pChild->drawable() == false)
		{
			bChildrenDrawable = false;
			break;
		}
	}

	if (bStayHere)
	{
		if (pChunk->getTerrain() != NULL && pChunk->getTerrain()->isOK())
		{
			pChunk->setDistance(
				(
					gChunkPosition.toVector3() -
					pContext->camera()->getGeoloc().toVector3()
					)
				.getMagnitude()
				);

			// Paint this chunk
			vChunkCollect.append(pChunk);

			// Get rid of unneeded water
			if (pChunk->getWater() != NULL && pChunk->getWater()->getAllHeightsOverSea())
			{
				delete pChunk->getWater();
				pChunk->setWater(NULL);
			}

			// Get rid of empty nodes
			for (int iIndex = 0; iIndex < pChunk->getChildren().count(); iIndex++)
			{
				CWorldChunk* pChild = dynamic_cast<CWorldChunk*>(pChunk->getChildren()[iIndex]);

				if (pChild->isEmpty())
				{
					delete pChild;
					pChunk->getChildren().remove(iIndex);
					iIndex--;
				}
			}
		}
		else if (bChildrenDrawable)
		{
			// Paint children
			foreach (CComponent* pChildComponent, pChunk->getChildren())
			{
				CWorldChunk* pChild = dynamic_cast<CWorldChunk*>(pChildComponent);

				paintRecurse(vChunkCollect, pContext, pChild, iLevel - 1, false);
			}
		}
	}
	else
	{
		if (bChildrenDrawable)
		{
			// Paint children
			foreach (CComponent* pChildComponent, pChunk->getChildren())
			{
				CWorldChunk* pChild = dynamic_cast<CWorldChunk*>(pChildComponent);

				paintRecurse(vChunkCollect, pContext, pChild, iLevel - 1, false);
			}
		}
		else
		{
			pChunk->setDistance(
				(
					gChunkPosition.toVector3() -
					pContext->camera()->getGeoloc().toVector3()
					)
				.getMagnitude()
				);

			// Paint this chunk
			vChunkCollect.append(pChunk);
		}
	}
}

//-------------------------------------------------------------------------------------------------

void CAutoTerrain::collectGarbage()
{
	if (m_pRoot != NULL)
	{
		collectGarbageRecurse(m_pRoot);
	}
}

//-------------------------------------------------------------------------------------------------

void CAutoTerrain::collectGarbageRecurse(CWorldChunk* pChunk)
{
	if (pChunk->getTerrain() != NULL)
	{
		if (pChunk->getTerrain()->isOK() && pChunk->getLastUsed().secsTo(QDateTime::currentDateTime()) > 60.0)
		{
			pChunk->clearTerrain();
		}
	}

	foreach (CComponent* pChildComponent, pChunk->getChildren())
	{
		CWorldChunk* pChild = dynamic_cast<CWorldChunk*>(pChildComponent);

		collectGarbageRecurse(pChild);
	}
}

//-------------------------------------------------------------------------------------------------

double CAutoTerrain::getHeightAt(const CGeoloc& gPosition, double* pRigidness)
{
	if (m_pRoot != NULL)
	{
		return getHeightAtRecurse(gPosition, m_pRoot, pRigidness);
	}

	return Q3D_INFINITY;
}

//-------------------------------------------------------------------------------------------------

double CAutoTerrain::getHeightAtRecurse(const CGeoloc& gPosition, CWorldChunk* pChunk, double* pRigidness)
{
	double dDiffLatitude = Math::Angles::angleDifferenceDegree(gPosition.Latitude, pChunk->getGeoloc().Latitude);
	double dDiffLongitude = Math::Angles::angleDifferenceDegree(gPosition.Longitude, pChunk->getGeoloc().Longitude);

	if (pRigidness) *pRigidness = 0.0;

	if (
		fabs(dDiffLatitude) < pChunk->getSize().Latitude * 0.5 &&
		fabs(dDiffLongitude) < pChunk->getSize().Longitude * 0.5
		)
	{
		foreach (CComponent* pChildComponent, pChunk->getChildren())
		{
			CWorldChunk* pChild = dynamic_cast<CWorldChunk*>(pChildComponent);

			double dNewRigidness = 0.0;
			double dNewAltitude = getHeightAtRecurse(gPosition, pChild, &dNewRigidness);

			if (dNewAltitude != Q3D_INFINITY)
			{
				if (pRigidness) *pRigidness = dNewRigidness;
				return dNewAltitude;
			}
		}

		if (pChunk->getTerrain() != NULL && pChunk->getTerrain()->isOK())
		{
			double dTerrainRigidness = 0.0;
			double dTerrainAltitude = pChunk->getTerrain()->getHeightAt(gPosition, &dTerrainRigidness);

			if (pChunk->getWater() != NULL)
			{
				double dWaterRigidness = 0.0;
				double dWaterAltitude = pChunk->getWater()->getHeightAt(gPosition, &dWaterRigidness);

				if (dWaterAltitude != Q3D_INFINITY && dWaterAltitude > dTerrainAltitude)
				{
					dTerrainRigidness = dWaterRigidness;
					dTerrainAltitude = dWaterAltitude;
				}
			}

			if (dTerrainAltitude != Q3D_INFINITY)
			{
				if (pRigidness) *pRigidness = dTerrainRigidness;
				return dTerrainAltitude;
			}

		}
	}

	return Q3D_INFINITY;
}

//-------------------------------------------------------------------------------------------------

void CAutoTerrain::flatten(const CGeoloc& gPosition, double dRadius)
{
	/*
	foreach (ChunkMap map, m_vChunks)
	{
		if (map.pChunk)
		{
			if (map.pChunk->getWorldBounds().contains(gPosition, dRadius))
			{
				map.pChunk->flatten(gPosition, dRadius);
			}
		}
	}
	*/
}

//-------------------------------------------------------------------------------------------------

void CAutoTerrain::readVegetationParameters()
{
    CXMLNode xVegeationNode = m_xParameters.getNodeByTagName(ParamName_Vegetation);

    QVector<CXMLNode> xTrees = xVegeationNode.getNodesByTagName(ParamName_Tree);

	foreach (CXMLNode xTree, xTrees)
	{
        CXMLNode xDNA = xTree.getNodeByTagName(ParamName_DNA);
        CXMLNode xCoverage = xTree.getNodeByTagName(ParamName_Coverage);

        CGenerateFunction* pFunction = new CGenerateFunction(xCoverage.getNodeByTagName(ParamName_Value));

        int iLevels = xDNA.m_vAttributes[ParamName_Levels].toDouble();
        double dTrunkLength = xDNA.m_vAttributes[ParamName_TrunkLength].toDouble();
        double dTrunkRadius = xDNA.m_vAttributes[ParamName_TrunkRadius].toDouble();
        double dBranchLengthScale = xDNA.m_vAttributes[ParamName_BranchLengthScale].toDouble();
        double dBranchRadiusScale = xDNA.m_vAttributes[ParamName_BranchRadiusScale].toDouble();
        double dLeafScale = xDNA.m_vAttributes[ParamName_LeafScale].toDouble();
        double dGravityFactor = xDNA.m_vAttributes[ParamName_GravityFactor].toDouble();
        double dSpread = xDNA.m_vAttributes[ParamName_Spread].toDouble();

        CXMLNode xLeaf = xDNA.getNodeByTagName(ParamName_Leaf);
        CXMLNode xFFD = xLeaf.getNodeByTagName(ParamName_FFD);

        QVector<CXMLNode> xPoints = xFFD.getNodesByTagName(ParamName_Point);

		QVector<CVector3> vFFDFrom;
		QVector<CVector3> vFFDTo;

		foreach (CXMLNode xPoint, xPoints)
		{
			vFFDFrom.append(CVector3(
				xPoint.m_vAttributes["fx"].toDouble(),
				xPoint.m_vAttributes["fy"].toDouble(),
				xPoint.m_vAttributes["fz"].toDouble()
				));

			vFFDTo.append(CVector3(
				xPoint.m_vAttributes["tx"].toDouble(),
				xPoint.m_vAttributes["ty"].toDouble(),
				xPoint.m_vAttributes["tz"].toDouble()
				));
		}

		QVector<QSharedPointer<CMesh> > vMeshes;

		CVector3 vNoisePosition(
			((double) rand() / 32768.0) * 2.0,
			((double) rand() / 32768.0) * 2.0,
			((double) rand() / 32768.0) * 2.0
			);

		for (int iLODLevel = 0; iLODLevel < 5; iLODLevel++)
		{
			vMeshes.append(QSharedPointer<CMesh>(m_pScene->getTreeGenerator()->createTree(
				iLODLevel,
				vNoisePosition,
				iLevels,
				dTrunkLength,
				dTrunkRadius,
				dBranchLengthScale,
				dBranchRadiusScale,
				dLeafScale,
				dGravityFactor,
				vFFDFrom,
				vFFDTo
				)));
		}

		m_vVegetation.append(new CVegetation(dSpread, pFunction, new CMeshInstance(vMeshes)));
	}
}

//-------------------------------------------------------------------------------------------------

RayTracingResult CAutoTerrain::intersect(Math::CRay3 ray) const
{
	if (m_pRoot != NULL)
	{
		return intersectRecurse(m_pRoot, ray);
	}

	return RayTracingResult(Q3D_INFINITY);
}

//-------------------------------------------------------------------------------------------------

RayTracingResult CAutoTerrain::intersectRecurse(CWorldChunk* pChunk, const Math::CRay3& ray) const
{
	RayTracingResult dResult(Q3D_INFINITY);

	foreach (CComponent* pChildComponent, pChunk->getChildren())
	{
		CWorldChunk* pChild = dynamic_cast<CWorldChunk*>(pChildComponent);

		if (pChild)
		{
			RayTracingResult dNewResult = intersectRecurse(pChild, ray);

			if (dNewResult.m_dDistance < dResult.m_dDistance)
			{
				dResult = dNewResult;
			}
		}
	}

	RayTracingResult dNewResult = pChunk->intersect(ray);

	if (dNewResult.m_dDistance < dResult.m_dDistance)
	{
		dResult = dNewResult;
	}

	return dResult;
}

//-------------------------------------------------------------------------------------------------

void CAutoTerrain::dump(QTextStream& stream, int iIdent)
{
	dumpIdent(stream, iIdent, QString("[CAutoTerrain]"));
	dumpIdent(stream, iIdent, QString("Levels : %1").arg(m_iLevels));
	dumpIdent(stream, iIdent, QString("Terrain res : %1").arg(m_iTerrainResolution));
	dumpIdent(stream, iIdent, QString("Root :"));

	if (m_pRoot != NULL)
	{
		dumpOpenBlock(stream, iIdent); iIdent++;
		m_pRoot->dump(stream, iIdent);
		iIdent--; dumpCloseBlock(stream, iIdent);
	}

	CComponent::dump(stream, iIdent);
}
