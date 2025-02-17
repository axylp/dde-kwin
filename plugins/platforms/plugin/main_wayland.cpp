// Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vtablehook.h"
#include "kwinutils.h"
#include "kwinutilsadaptor.h"

#include <qpa/qplatformintegrationplugin.h>
#include <qpa/qplatformintegrationfactory_p.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>

#include <QDebug>
#include <QProcess>
#include <QPluginLoader>
#include <QDir>
#include <QRect>
#include <QQuickItem>
#include <QQuickWindow>
#include <QDBusConnection>

//#include "libkwinpreload.h"

#include "wm_interface.h"

QT_BEGIN_NAMESPACE

#define KWinUtilsDbusService "org.kde.KWin"
#define KWinUtilsDbusPath "/dde"
#define KWinUtilsDbusInterface "org.kde.KWin"

class Mischievous;
class Mischievous : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *workspace READ workspace)
    Q_PROPERTY(QObject *scripting READ scripting)
    Q_PROPERTY(KWinUtils *kwinUtils READ kwinUtils)
public:
    explicit Mischievous() {
        self = this;
    }

    QObject *workspace() const
    {
        return KWinUtils::workspace();
    }

    QObject *scripting() const
    {
        return KWinUtils::scripting();
    }

    KWinUtils *kwinUtils() const
    {
        return KWinUtils::instance();
    }

    Q_INVOKABLE QObject *require(const QString &module)
    {
        if (QObject *obj = moduleMap.value(module)) {
            return obj;
        }

        QString file = module;
        bool isFile = QFile::exists(file);

        if (!isFile) {
            static QStringList pluginPaths {
                QDir::home().absoluteFilePath(QStringLiteral(".local/lib/" PROJECT_NAME "/plugins")),
                QStringLiteral("/usr/lib/" PROJECT_NAME "/plugins")
            };

            for (const QString &path : pluginPaths) {
                QDir dir(path);

                file.prepend("lib");
                file.append(".so");
                file = dir.absoluteFilePath(file);
                isFile = QFile::exists(file);

                if (isFile) {
                    break;
                }
            }
        }

        if (!isFile) {
            return nullptr;
        }

        QPluginLoader loader(file);

        if (!loader.load()) {
            // qWarning() << Q_FUNC_INFO << loader.errorString();
            return nullptr;
        }

        QObject *obj = loader.instance();
        moduleMap[module] = obj;

        if (obj) {
            obj->metaObject()->invokeMethod(obj, "init", Q_ARG(QObject*, this));
        }

        return obj;
    }

    Q_INVOKABLE int execute(const QString &program, const QStringList &arguments, const QString &workingDirectory = QString())
    {
        QProcess p;

        p.setProgram(program);
        p.setArguments(arguments);
        p.setWorkingDirectory(workingDirectory);

        p.start(QIODevice::ReadOnly);
        p.waitForFinished();

        return p.exitCode();
    }
    Q_INVOKABLE int execute(const QString &command, const QString &workingDirectory = QString())
    {
        QProcess p;

        p.setWorkingDirectory(workingDirectory);
        p.start(command, QIODevice::ReadOnly);
        p.waitForFinished();

        return p.exitCode();
    }
    Q_INVOKABLE bool startDetached(const QString &program, const QStringList &arguments, const QString &workingDirectory = QString())
    {
        return QProcess::startDetached(program, arguments, workingDirectory);
    }
    Q_INVOKABLE bool startDetached(const QString &command)
    {
        return QProcess::startDetached(command);
    }

    Q_INVOKABLE bool setObjectProperty(QObject *obj, const QString &name, const QVariant &value)
    {
        return obj->setProperty(name.toLatin1().constData(), value);
    }

public slots:
    void init() {

        if (!KWinUtils::scripting())
            return;

        const QObjectList scripting_children = KWinUtils::scripting()->children();
        QObject *jsWorkspaceWrapper = KWinUtils::findObjectByClassName(QByteArrayLiteral("KWin::QtScriptWorkspaceWrapper"), scripting_children);
        QObject *qmlWorkspaceWrapper = KWinUtils::findObjectByClassName(QByteArrayLiteral("KWin::DeclarativeScriptWorkspaceWrapper"), scripting_children);

        // 给js脚本引擎添加对象
        if (jsWorkspaceWrapper) {
            jsWorkspaceWrapper->setProperty("__dde__", QVariant::fromValue(this));
        }

        // 给qml脚本引擎添加对象
        if (qmlWorkspaceWrapper) {
            qmlWorkspaceWrapper->setProperty("__dde__", QVariant::fromValue(this));
        }

        KWinUtils::scriptingRegisterObject(QStringLiteral("dde"), this);

        // 注册 dbus 对象 提供更多的 kwin 相关接口
        new KWinAdaptor(kwinUtils());
        QDBusConnection::sessionBus().registerObject(KWinUtilsDbusPath, KWinUtilsDbusInterface, kwinUtils());

        if (QObject *cursor = kwinUtils()->cursor()) {
            connect(cursor, SIGNAL(themeChanged()), this, SLOT(onCursorThemeChanged()), Qt::QueuedConnection);
        }

        // 初始化翻译资源
        QTranslator *ts = new QTranslator(this);
        const QString &lang_name = QLocale::system().name();
        QString ts_file = TRANSLATE_NAME "_" + lang_name;
        QString ts_fallback_file;

        {
            int index = lang_name.indexOf("_");

            if (index > 0) {
                ts_fallback_file = lang_name.left(index);
            }
        }

        auto ts_dir_list = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);

        while (!ts_file.isEmpty()) {
            bool ok = false;

            for (QString dir : ts_dir_list) {
                dir += "/" TRANSLATE_NAME "/translations";

                if (!QDir(dir).exists()) {
                    continue;
                }

                if (ts->load(ts_file, dir) && qApp->installTranslator(ts)) {
                    ok = true;
                    break;
                } else {
                    // qWarning() << Q_FUNC_INFO << "Failed on load translators, file:" << dir + "/" + ts_file;
                }
            }

            ts_file.clear();

            if (!ok && !ts_fallback_file.isEmpty()) {
                ts_file = ts_fallback_file;
                ts_fallback_file.clear();
            }
        }

        // 通知程序初始化完成
        kwinUtils()->setInitialized();
    }

    void onExec() {
        if (KWinUtils::scripting()) {
            init();
        } else {
            connect(qApp, SIGNAL(workspaceCreated()), this, SLOT(init()));
        }
    }

    void updateCursorSize() {
        bool ok = false;
        int cursorSize = 0;
        //qdbus默认会使用25s超时机制，这个机制在某些实时性要求较高的场景并不太适用。
        //设置超时时间为200ms，超时尝试最大5次，综合下来能够达到即不会阻塞太长时间，重试也能增强逻辑的兼容性。
        int retry = 5;
        int timeout = 200;
        
        ComDeepinWmInterface wm_interface("com.deepin.wm",
                                          "/com/deepin/wm",
                                          QDBusConnection::sessionBus(), this);

        //设置超时时间,超时后，进行重试
        wm_interface.setTimeout(timeout);
        while(retry--) {
            cursorSize = wm_interface.cursorSize();
            if (!wm_interface.lastError().isValid()) {
                // 读取数据成功
                ok = true;
                break;
            }
        }
        
        // 应该跟随dpi缩放设置光标大小
        if (!ok || cursorSize <= 0) {
            if (QScreen *s = QGuiApplication::primaryScreen()) {
                cursorSize = qRound(s->logicalDotsPerInchY() * 16 / 72);
                qputenv("XCURSOR_SIZE", QByteArray::number(cursorSize));
            }
        }
    }

    void onCursorThemeChanged() {
        updateCursorSize();

        // 光标主题改变后应该立即更新所有客户端的当前光标
        for (QObject *client : kwinUtils()->clientList()) {
            QMetaObject::invokeMethod(client, "moveResizeCursorChanged", Q_ARG(Qt::CursorShape, Qt::ArrowCursor));
            const QVariant &wrapper = kwinUtils()->getParentWindow(client);

            // KWin会为client创建一个父窗口wrapper，wrapper初始化时设置了光标为ArrowCursor类型,
            // gtk应用默认不会给主窗口设置任何光标，因此gtk窗口会跟随其父窗口wrapper的光标样式，
            // 当光标主题改变时，应该主动更新wrapper的光标，否则会导致gtk应用的窗口默认光标不跟随主题
            if (wrapper.isValid())
                KWinUtils::defineWindowCursor(wrapper.toUInt(), Qt::ArrowCursor);
        }
    }

public:
    static Mischievous *self;
    QMap<QString, QObject*> moduleMap;
};

Mischievous *Mischievous::self = nullptr;
Q_GLOBAL_STATIC(Mischievous, _m)

static void overrideInitialize(QPlatformIntegration *i)
{
    *QGuiApplicationPrivate::platform_name = "wayland";
    VtableHook::callOriginalFun(i, &QPlatformIntegration::initialize);
}

class DKWinWaylandPlatformIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "dde-kwin-wayland.json")

public:
    QPlatformIntegration *create(const QString&, const QStringList&, int &, char **) Q_DECL_OVERRIDE;
};

QPlatformIntegration* DKWinWaylandPlatformIntegrationPlugin::create(const QString& system, const QStringList& parameters, int &argc, char **argv)
{
    if (system == TARGET_NAME) {
        // 清理对libdde-kwin-wayland.so的ld preload，防止使用QProcess调用其他进程时被传递
        qunsetenv("LD_PRELOAD");

        QPlatformIntegration *integration;

        integration = QPlatformIntegrationFactory::create("wayland-org.kde.kwin.qpa", parameters, argc, argv, PLATFORMS_PLUGIN_PATH);

        VtableHook::overrideVfptrFun(integration, &QPlatformIntegration::initialize, overrideInitialize);
        QMetaObject::invokeMethod(_m.operator ->(), "onExec", Qt::QueuedConnection);

        return integration;
    }

    return 0;
}

QT_END_NAMESPACE

#include "main_wayland.moc"
