add_definitions(-DTRANSLATION_DOMAIN=\"plasma_wallpaper_org.kde.plasma.plasmawallpaper\")

set(plasmawallpaperplugin_SRCS
    plasmawallpaperplugin.cpp
)

add_library(plasmawallpaperplugin SHARED ${plasmawallpaperplugin_SRCS})

target_link_libraries(plasmawallpaperplugin
    KF5::I18n
    Qt5::Gui
    Qt5::Qml
)
install(TARGETS plasmawallpaperplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/plasmawallpaper)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/plasmawallpaper)
