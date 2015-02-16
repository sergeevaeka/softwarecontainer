#include "pelagicontain-lib.h"

#include "generators.h" /* used for gen_ct_name */

#ifdef ENABLE_PULSEGATEWAY
#include "pulsegateway.h"
#endif

#ifdef ENABLE_NETWORKGATEWAY
#include "networkgateway.h"
#endif

#ifdef ENABLE_DBUSGATEWAY
#include "dbusgateway.h"
#endif

#ifdef ENABLE_DEVICENODEGATEWAY
#include "devicenodegateway.h"
#endif

//#include "dltgateway.h"
#include "envgateway.h"
#include "waylandgateway.h"
#include "filegateway.h"

#include "config.h"


PelagicontainLib::PelagicontainLib(const char *containerRootFolder, const char *configFilePath) :
    containerConfig(configFilePath), containerRoot(containerRootFolder),
    container(getContainerID(), m_containerName, containerConfig, containerRoot)
{
    pelagicontain.setMainLoopContext(m_ml);
}

PelagicontainLib::~PelagicontainLib()
{
}

void PelagicontainLib::setContainerIDPrefix(const std::string &name)
{
    m_containerID = name + Generator::gen_ct_name();
    log_debug() << "Assigned container ID " << m_containerID;
}

void PelagicontainLib::setContainerName(const std::string &name)
{
    m_containerName = name;
    log_debug() << container.toString();
}

void PelagicontainLib::validateContainerID()
{
    if (m_containerID.size() == 0) {
        setContainerIDPrefix("PLC-");
    }
}

ReturnCode PelagicontainLib::checkWorkspace()
{
    if ( !isDirectory(containerRoot) ) {
        std::string cmdLine = INSTALL_PREFIX;
        cmdLine += "/bin/setup_pelagicontain.sh " + containerRoot;
        log_debug() << "Creating workspace : " << cmdLine;
        int returnCode;
        try {
            Glib::spawn_sync("", Glib::shell_parse_argv(
                        cmdLine), static_cast<Glib::SpawnFlags>(0) /*value available as Glib::SPAWN_DEFAULT in recent glibmm*/,
                    sigc::slot<void>(), nullptr,
                    nullptr, &returnCode);
        } catch (Glib::SpawnError e) {
            log_error() << "Failed to spawn " << cmdLine << ": code " << e.code() << " msg: " << e.what();
            return ReturnCode::FAILURE;
        }
        if (returnCode != 0) {
            log_error() << "Return code of " << cmdLine << " is non-zero";
            return ReturnCode::FAILURE;
        }
    }

    return ReturnCode::SUCCESS;
}

ReturnCode PelagicontainLib::preload()
{

    // Make sure path ends in '/' since it might not always be checked
    if (containerRoot.back() != '/') {
        containerRoot += "/";
    }

    if ( isError( checkWorkspace() ) ) {
        log_error() << "Failed when checking workspace";
        return ReturnCode::FAILURE;
    }

    if ( isError( container.initialize() ) ) {
        log_error() << "Could not setup container for preloading";
        return ReturnCode::FAILURE;
    }

    m_pcPid = pelagicontain.preload(container);

    if (m_pcPid != 0) {
        log_debug() << "Started container with PID " << m_pcPid;
    } else {
        // Fatal failure, only do necessary cleanup
        log_error() << "Could not start container, will shut down";
    }

    return ReturnCode::SUCCESS;
}

ReturnCode PelagicontainLib::init()
{
    validateContainerID();

    if (m_ml->gobj() == nullptr) {
        log_error() << "Main loop context must be set first !";
        return ReturnCode::FAILURE;
    }

    if (pelagicontain.getContainerState() != ContainerState::PRELOADED) {
        if ( isError( preload() ) ) {
            log_error() << "Failed to preload container";
            return ReturnCode::FAILURE;
        }
    }

    //    if (bRegisterDBusInterface) {
    //        if (m_cookie.size() == 0) {
    //            m_cookie = containerName;
    //        }
    //    }
    //
    //    DBus::default_dispatcher = &dispatcher;
    //
    //    m_bus = new DBus::Connection( DBus::Connection::SessionBus() );
    //    dispatcher.attach( m_ml->gobj() );

#ifdef ENABLE_NETWORKGATEWAY
    m_gateways.push_back( std::unique_ptr<Gateway>( new NetworkGateway() ) );
#endif

#ifdef ENABLE_PULSEGATEWAY
    m_gateways.push_back( std::unique_ptr<Gateway>( new PulseGateway( getGatewayDir(), getContainerID() ) ) );
#endif

#ifdef ENABLE_DEVICENODEGATEWAY
    m_gateways.push_back( std::unique_ptr<Gateway>( new DeviceNodeGateway() ) );
#endif

#ifdef ENABLE_DBUSGATEWAY
    m_gateways.push_back( std::unique_ptr<Gateway>( new DBusGateway(
                    DBusGateway::SessionProxy,
                    getGatewayDir(),
                    getContainerID() ) ) );

    m_gateways.push_back( std::unique_ptr<Gateway>( new DBusGateway(
                    DBusGateway::SystemProxy,
                    getGatewayDir(),
                    getContainerID() ) ) );
#endif

    //    m_gateways.push_back( std::unique_ptr<Gateway>( new DLTGateway() ) );
    m_gateways.push_back( std::unique_ptr<Gateway>( new WaylandGateway() ) );
    m_gateways.push_back( std::unique_ptr<Gateway>( new FileGateway() ) );
    m_gateways.push_back( std::unique_ptr<Gateway>( new EnvironmentGateway() ) );

    for (auto &gateway : m_gateways) {
        pelagicontain.addGateway(*gateway);
    }

    // The pid might not valid if there was an error spawning. We should only
    // connect the watcher if the spawning went well.
    if (m_pcPid != 0) {
        addProcessListener(m_connections, m_pcPid, [&] (pid_t pid, int exitCode) {
                    pelagicontain.shutdownContainer();
                }, m_ml);
    }

    //    if (bRegisterDBusInterface) {
    //        registerDBusService();
    //    }

    m_initialized = true;

    return ReturnCode::SUCCESS;
}


//ReturnCode PelagicontainLib::registerDBusService()
//{
//    /* The request_name call does not return anything but raises an
//     * exception if the name cannot be requested.
//     */
//    std::string name = "com.pelagicore.Pelagicontain" + m_cookie;
//    m_bus->request_name( name.c_str() );
//
//    std::string objectPath = "/com/pelagicore/Pelagicontain";
//
//    log_debug() << "Registering interface on DBUS";
//
//    m_pcAdapter = std::unique_ptr<PelagicontainToDBusAdapter> ( new PelagicontainToDBusAdapter(*m_bus, objectPath, pelagicontain) );
//
//    return ReturnCode::SUCCESS;
//}

void PelagicontainLib::openTerminal(const std::string &terminalCommand) const
{
    std::string command = logging::StringBuilder() << terminalCommand << " lxc-attach -n " << container.id();
    log_info() << command;
    system( command.c_str() );
}
