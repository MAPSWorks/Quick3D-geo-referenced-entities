
#pragma once

// Application
#include "quick3d_global.h"
#include "CQ3DConstants.h"
#include "CElectricalComponent.h"

//-------------------------------------------------------------------------------------------------

class C3DScene;

class QUICK3D_EXPORT CElectricalBus : public CElectricalComponent
{
public:

	//-------------------------------------------------------------------------------------------------
	// Constructors and destructor
	//-------------------------------------------------------------------------------------------------

	//!
	static CComponent* instanciator(C3DScene* pScene);

	//!
	CElectricalBus(C3DScene* pScene);

	//!
	virtual ~CElectricalBus();

	//-------------------------------------------------------------------------------------------------
	// Setters
	//-------------------------------------------------------------------------------------------------

	//-------------------------------------------------------------------------------------------------
	// Getters
	//-------------------------------------------------------------------------------------------------

	//-------------------------------------------------------------------------------------------------
	// Inherited methods
	//-------------------------------------------------------------------------------------------------

	//!
	virtual QString getClassName() const { return ClassName_CElectricalBus; }

	//-------------------------------------------------------------------------------------------------
	// Control methods
	//-------------------------------------------------------------------------------------------------

	//-------------------------------------------------------------------------------------------------
	// Properties
	//-------------------------------------------------------------------------------------------------

protected:
};
