#include "propertyBrowserController.h"
#include "../tools/propertyManagers/property.h"
#include "../tools/propertyManagers/propertyManager.h"

using namespace qReal::widgetsEdit;

PropertyBrowserController::PropertyBrowserController(QtTreePropertyBrowser *browser)
	: mBrowser(browser)
	, mFactory(new QtVariantEditorFactory)
	, mCurrentManager(NULL)
{
}

void PropertyBrowserController::setController(PropertyManager *manager)
{
	clearExistingManager();
	mCurrentManager = manager;
	mCurrentManager->onSelect();
	mBrowser->setFactoryForManager(mCurrentManager->qtPropertyManager(), mFactory);
	QListIterator<Property *> it = mCurrentManager->propertiesIterator();
	while (it.hasNext()) {
		mBrowser->addProperty(it.next()->qtProperty());
	}
}

void PropertyBrowserController::clearExistingManager()
{
	mBrowser->clear();
	if (mCurrentManager) {
		mBrowser->unsetFactoryForManager(mCurrentManager->qtPropertyManager());
	}
}
