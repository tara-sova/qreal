#include "interpreter.h"

#include <QtCore/QCoreApplication>
#include <QtWidgets/QAction>
#include <QtCore/QDebug>

#include <interpreterBase/robotModel/robotModelInterface.h>

#include "details/autoconfigurer.h"
//#include "details/tracer.h"
//#include "details/debugHelper.h"


/// @todo Temporary
#include <interpreterBase/robotModel/robotParts/touchSensor.h>

using namespace qReal;
using namespace interpreterCore::interpreter;
using namespace interpreterBase::robotModel;

int const maxThreadsCount = 100;

Interpreter::Interpreter(GraphicalModelAssistInterface const &graphicalModelApi
		, LogicalModelAssistInterface &logicalModelApi
		, qReal::gui::MainWindowInterpretersInterface &interpretersInterface
		, qReal::ProjectManagementInterface const &projectManager
		, BlocksFactoryManagerInterface &blocksFactoryManager
		, interpreterBase::robotModel::RobotModelManagerInterface const &robotModelManager
		, utils::ExpressionsParser &parser  /// @todo direct dependency from ExpressionsParser shall be removed.
		, QAction &connectToRobotAction
		)
	: mGraphicalModelApi(&graphicalModelApi)
	, mLogicalModelApi(&logicalModelApi)
	, mInterpretersInterface(&interpretersInterface)
	, mState(idle)
	, mRobotModelManager(robotModelManager)
	, mBlocksTable(nullptr)
	, mActionConnectToRobot(connectToRobotAction)
	, mSensorVariablesUpdater(robotModelManager, parser)
{
	mBlocksTable = new details::BlocksTable(blocksFactoryManager);

	/// @todo Reinit those connects when model changes, or add these slots to RobotModelManager and connect to them.
	connect(
			&mRobotModelManager
			, &interpreterBase::robotModel::RobotModelManagerInterface::allDevicesConfigured
			, this
			, &Interpreter::sensorsConfiguredSlot
			);

	connect(
			&mRobotModelManager
			, &interpreterBase::robotModel::RobotModelManagerInterface::connected
			, this
			, &Interpreter::connectedSlot
			);

	connect(&projectManager, &qReal::ProjectManagementInterface::beforeOpen, this, &Interpreter::stopRobot);

	qDebug() << "mRobotModelManager.model().init()";

	mRobotModelManager.model().init();
	mRobotModelManager.model().connectToRobot();
}

Interpreter::~Interpreter()
{
	qDeleteAll(mThreads);
	delete mBlocksTable;
}

void Interpreter::interpret()
{
	qDebug() << "Interpreter::interpret()";

	mInterpretersInterface->errorReporter()->clear();

//	Id const &currentDiagramId = mInterpretersInterface->activeDiagram();

	if (mRobotModelManager.model().connectionState() != RobotModelInterface::connectedState) {
		mInterpretersInterface->errorReporter()->addInformation(tr("No connection to robot"));
		return;
	}

	if (mState != idle) {
		mInterpretersInterface->errorReporter()->addInformation(tr("Interpreter is already running"));
		return;
	}

	mBlocksTable->clear();
	mState = waitingForSensorsConfiguredToLaunch;

	/// @todo Temporarily loading initial configuration from registry
	/// (actually, from a network of SensorConfigurationProviders). To be done more adequately.
	for (PortInfo const &port : mRobotModelManager.model().configurablePorts()) {
		QString const modelName = mRobotModelManager.model().name();
		if (mCurrentConfiguration[modelName].contains(port)) {
			DeviceInfo const &deviceInfo = mCurrentConfiguration[modelName].value(port);
			mRobotModelManager.model().configureDevice(port, deviceInfo);
		}
	}

	mRobotModelManager.model().applyConfiguration();

//	details::Autoconfigurer configurer(
//				*mGraphicalModelApi
//				, mBlocksTable
//				, mInterpretersInterface->errorReporter()
//				, &mRobotModelManager.model()
//				);

//	bool configurationSucceeded = false;
//	QVector<interpreterBase::robotModel::SensorId> const sensorConfiguration
//			= configurer.configure(currentDiagramId, &configurationSucceeded);

//	if (!configurationSucceeded) {
//		return;
//	}

//	mRobotModel->configureSensors(
//			sensorConfiguration[0]
//			, sensorConfiguration[1]
//			, sensorConfiguration[2]
//			, sensorConfiguration[3]
//			);
}

void Interpreter::stopRobot()
{
	mRobotModelManager.model().stopRobot();
	mState = idle;
	qDeleteAll(mThreads);
	mThreads.clear();
	mBlocksTable->setFailure();
}

void Interpreter::connectedSlot(bool success)
{
	qDebug() << "Interpreter::connectedSlot";

	if (success) {
		if (mRobotModelManager.model().needsConnection()) {
			mInterpretersInterface->errorReporter()->addInformation(tr("Connected successfully"));
		}
	} else {
//		Tracer::debug(tracer::enums::initialization, "Interpreter::connectedSlot", "Robot connection status: " + QString::number(success));
		mInterpretersInterface->errorReporter()->addError(tr("Can't connect to a robot."));
	}

	mActionConnectToRobot.setChecked(success);
}

void Interpreter::sensorsConfiguredSlot()
{
//	Tracer::debug(tracer::enums::initialization, "Interpreter::sensorsConfiguredSlot", "Sensors are configured");

	qDebug() << "Interpreter::sensorsConfiguredSlot";

//	mRobotModel->nextBlockAfterInitial(mConnected);

	if (mRobotModelManager.model().connectionState() != RobotModelInterface::connectedState) {
		mInterpretersInterface->errorReporter()->addInformation(tr("No connection to robot"));
		mState = idle;
		return;
	}

	if (mState == waitingForSensorsConfiguredToLaunch) {
		mState = interpreting;

		mSensorVariablesUpdater.run();

//		Tracer::debug(tracer::enums::initialization, "Interpreter::sensorsConfiguredSlot", "Starting interpretation");
//		mRobotModel->startInterpretation();

		Id const &currentDiagramId = mInterpretersInterface->activeDiagram();

		details::Thread * const initialThread = new details::Thread(mGraphicalModelApi
				, *mInterpretersInterface, currentDiagramId, *mBlocksTable);

		addThread(initialThread);
	}
}

void Interpreter::threadStopped()
{
	details::Thread * const thread = static_cast<details::Thread *>(sender());

	mThreads.removeAll(thread);
	delete thread;

	if (mThreads.isEmpty()) {
		stopRobot();
	}
}

void Interpreter::newThread(Id const &startBlockId)
{
	details::Thread * const thread = new details::Thread(mGraphicalModelApi
			, *mInterpretersInterface, *mBlocksTable, startBlockId);

	addThread(thread);
}

//void Interpreter::configureSensors(
//		robots::enums::sensorType::SensorTypeEnum const &port1
//		, robots::enums::sensorType::SensorTypeEnum const &port2
//		, robots::enums::sensorType::SensorTypeEnum const &port3
//		, robots::enums::sensorType::SensorTypeEnum const &port4)
//{
//}

void Interpreter::addThread(details::Thread * const thread)
{
	if (mThreads.count() >= maxThreadsCount) {
//		reportError(tr("Threads limit exceeded. Maximum threads count is %1").arg(maxThreadsCount));
		stopRobot();
	}

	mThreads.append(thread);
	connect(thread, SIGNAL(stopped()), this, SLOT(threadStopped()));

	connect(thread
			, SIGNAL(newThread(Id const &))
			, this, SLOT(newThread(Id const &))
			);

	QCoreApplication::processEvents();
	if (mState != idle) {
		thread->interpret();
	}
}

void Interpreter::connectToRobot()
{
	if (mState == interpreting) {
		return;
	}

	if (mRobotModelManager.model().connectionState() == RobotModelInterface::connectedState) {
		mRobotModelManager.model().stopRobot();
		mRobotModelManager.model().disconnectFromRobot();
	} else {
		mRobotModelManager.model().connectToRobot();
//		configureSensors(
//				  static_cast<robots::enums::sensorType::SensorTypeEnum>(SettingsManager::instance()->value("port1SensorType").toInt())
//				, static_cast<robots::enums::sensorType::SensorTypeEnum>(SettingsManager::instance()->value("port2SensorType").toInt())
//				, static_cast<robots::enums::sensorType::SensorTypeEnum>(SettingsManager::instance()->value("port3SensorType").toInt())
//				, static_cast<robots::enums::sensorType::SensorTypeEnum>(SettingsManager::instance()->value("port4SensorType").toInt()));
	}

	mActionConnectToRobot.setChecked(
			mRobotModelManager.model().connectionState() == RobotModelInterface::connectedState);
}

void Interpreter::disconnectSlot()
{
	mActionConnectToRobot.setChecked(false);
	stopRobot();
}

//void Interpreter::setRobotModelType(robots::enums::robotModelType::robotModelTypeEnum robotModelType)
//{
//	setRobotImplementation(robotModelType);
//}

//void Interpreter::reportError(QString const &message)
//{
//	mInterpretersInterface->errorReporter()->addError(message);
//}

//void Interpreter::saveSensorConfiguration()
//{
//	Id const currentDiagramId = mInterpretersInterface->activeDiagram();
//	Id const logicalId = mGraphicalModelApi->logicalId(currentDiagramId);
//	if (logicalId != Id() && logicalId != Id::rootId()) {
//		int const sensor1Value = SettingsManager::value("port1SensorType").toInt();
//		int const sensor2Value = SettingsManager::value("port2SensorType").toInt();
//		int const sensor3Value = SettingsManager::value("port3SensorType").toInt();
//		int const sensor4Value = SettingsManager::value("port4SensorType").toInt();
//		mLogicalModelApi->setPropertyByRoleName(logicalId, QString::number(sensor1Value), "sensor1Value");
//		mLogicalModelApi->setPropertyByRoleName(logicalId, QString::number(sensor2Value), "sensor2Value");
//		mLogicalModelApi->setPropertyByRoleName(logicalId, QString::number(sensor3Value), "sensor3Value");
//		mLogicalModelApi->setPropertyByRoleName(logicalId, QString::number(sensor4Value), "sensor4Value");
//	}
//}

//void Interpreter::loadSensorConfiguration(Id const &diagramId)
//{
//	int const oldSensor1Value = SettingsManager::value("port1SensorType").toInt();
//	int const oldSensor2Value = SettingsManager::value("port2SensorType").toInt();
//	int const oldSensor3Value = SettingsManager::value("port3SensorType").toInt();
//	int const oldSensor4Value = SettingsManager::value("port4SensorType").toInt();

//	int const sensor1Value = mLogicalModelApi->propertyByRoleName(diagramId, "sensor1Value").toInt();
//	int const sensor2Value = mLogicalModelApi->propertyByRoleName(diagramId, "sensor2Value").toInt();
//	int const sensor3Value = mLogicalModelApi->propertyByRoleName(diagramId, "sensor3Value").toInt();
//	int const sensor4Value = mLogicalModelApi->propertyByRoleName(diagramId, "sensor4Value").toInt();

//	bool const somethingChanged = oldSensor1Value != sensor1Value
//			|| oldSensor2Value != sensor2Value
//			|| oldSensor3Value != sensor3Value
//			|| oldSensor4Value != sensor4Value;

//	SettingsManager::setValue("port1SensorType", sensor1Value);
//	SettingsManager::setValue("port2SensorType", sensor2Value);
//	SettingsManager::setValue("port3SensorType", sensor3Value);
//	SettingsManager::setValue("port4SensorType", sensor4Value);

//	if (somethingChanged) {
//		emit sensorsConfigurationChanged();
//	}
//}

qReal::IdList Interpreter::providedBlocks() const
{
	return mBlocksTable->providedBlocks();
}