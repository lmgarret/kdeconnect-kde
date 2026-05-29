kdeconnect_add_plugin(kdeconnect_share SOURCES shareplugin.cpp)
target_link_libraries(kdeconnect_share
    kdeconnectcore
    Qt::DBus
    KF6::Notifications
    KF6::I18n
    KF6::GuiAddons
    KF6::Service
)

if(HAVE_KIO)
    target_link_libraries(kdeconnect_share KF6::KIOWidgets)
endif()

install(FILES "kdeconnect_share_config.qml" DESTINATION ${KDE_INSTALL_DATADIR}/kdeconnect)
