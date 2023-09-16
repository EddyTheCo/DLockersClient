#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "Qrimageprovider.hpp"
#include"mydesigns.hpp"
#include"Day_model.hpp"
#include"Qrimagedecoder.hpp"
int main(int argc, char *argv[])
{
    auto foo=fooDesign::fooPrint(); //unused function to force linking
    foo=fooBookingModel::fooPrint(); //unused function to force linking
    foo=fooQtQrDec::fooPrint(); //unused function to force linking see https://forum.qt.io/topic/145975/qml-module-of-only-qml-files-do-not-link-to-application?_=1694679872016

	QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.addImportPath("qrc:/esterVtech.com/imports");
    engine.addImageProvider(QLatin1String("qrcodeblack"), new QRImageProvider("black",1));
#ifdef USE_EMSCRIPTEN
    engine.addImageProvider(QLatin1String("wasm"), new WasmImageProvider());
#endif
    qDebug()<<engine.importPathList();

    qmlRegisterSingletonType(QUrl(u"qrc:/esterVtech.com/imports/MyDesigns/qml/CustomStyle.qml"_qs), "CustomStyle", 1, 0, "CustomStyle");
    const QUrl url(u"qrc:/esterVtech.com/imports/client/qml/window.qml"_qs);
	QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
			&app, [url](QObject *obj, const QUrl &objUrl) {
			if (!obj && url == objUrl)
			QCoreApplication::exit(-1);
			}, Qt::QueuedConnection);

	engine.load(url);
	return app.exec();
}
