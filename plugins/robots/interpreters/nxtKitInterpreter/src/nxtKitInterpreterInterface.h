#pragma once

#include <interpreterBase/kitPluginInterface.h>

namespace nxtKitInterpreter {

class NxtKitInterpreterInterface : public QObject, public interpreterBase::KitPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(interpreterBase::KitPluginInterface)
	Q_PLUGIN_METADATA(IID "nxtKitInterpreter.NxtKitInterpreterInterface")

public:
	QString kitId() const override;

	QString friendlyKitName() const override;

	interpreterBase::robotModel::RobotModelInterface *realRobotModel() override;

	interpreterBase::robotModel::RobotModelInterface *twoDRobotModel() override;

	// Transfers ownership.
	QWidget *settingsWidget() const override;

//	qReal::IdList specificBlocks() const override;

	// Override.
	qReal::IdList unsupportedBlocks() const override;
};

}