#include "rtimvMainWindow.hpp"

#define RTIMV_DEBUG_BREADCRUMB

rtimvMainWindow::rtimvMainWindow( int argc, char **argv, QWidget *Parent, Qt::WindowFlags f ) : rtimvBase( Parent, f )
{
    m_configPathCLBase_env = "RTIMV_CONFIG_PATH"; // Tells mx::application to look for this env var.

    setup( argc, argv );

    if( doHelp )
    {
        help();
        exit( 0 );
    }

    ui.setupUi( this );

    m_northArrow = 0;

    imcp = 0;
    pointerOverZoom = 4.;

    resize( height(), height() ); // make square.

    // This will come up at some minimal size.
    ui.graphicsView->setGeometry( 0, 0, width(), height() );

    m_qgs = new QGraphicsScene();
    m_qgs->installEventFilter( this );

    ui.graphicsView->setScene( m_qgs );

    rightClickDragging = false;

    m_nullMouseCoords = true;

    mindat( 400 );

    maxdat( 600 );

    m_targetVisible = false;

    m_cenLineVert = 0;
    m_cenLineHorz = 0;

    imStats = 0;
    m_imageTimer.start( m_imageTimeout );

    m_northArrow = m_qgs->addLine( QLineF( 512, 400, 512, 624 ), QColor( ui.graphicsView->gageFontColor() ) );
    m_northArrowTip = m_qgs->addLine( QLineF( 512, 400, 536, 424 ), QColor( ui.graphicsView->gageFontColor() ) );
    m_northArrow->setTransformOriginPoint( QPointF( 512, 512 ) );
    m_northArrowTip->setTransformOriginPoint( QPointF( 512, 512 ) );
    m_northArrow->setVisible( false );
    m_northArrowTip->setVisible( false );

    QPen qp = m_northArrow->pen();
    qp.setWidth( 5 );

    m_northArrow->setPen( qp );
    m_northArrowTip->setPen( qp );

    m_lineHead = new QGraphicsEllipseItem;
    m_lineHead->setVisible( false );
    m_qgs->addItem( m_lineHead );

    m_objCenV = new QGraphicsLineItem;
    m_objCenV->setVisible( false );
    m_qgs->addItem( m_objCenV );

    m_objCenH = new QGraphicsLineItem;
    m_objCenH->setVisible( false );
    m_qgs->addItem( m_objCenH );

    /* ========================================= */
    /* now load plugins                          */
    /* ========================================= */

    /* -- static plugins -- */
    const auto staticInstances = QPluginLoader::staticInstances();

    for( QObject *plugin : staticInstances )
    {
        static_cast<void>( plugin );
        std::cerr << "loaded static plugins\n";
    }

    QDir pluginsDir = QDir( QCoreApplication::applicationDirPath() );

#if defined( Q_OS_WIN )
    if( pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release" )
    {
        pluginsDir.cdUp();
    }
#elif defined( Q_OS_MAC )
    if( pluginsDir.dirName() == "MacOS" )
    {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif

    if( pluginsDir.cd( "plugins" ) )
    {
        const auto entryList = pluginsDir.entryList( QDir::Files );

        for( const QString &fileName : entryList )
        {
            QPluginLoader loader( pluginsDir.absoluteFilePath( fileName ) );
            QObject *plugin = loader.instance();
            if( plugin )
            {
                int arv = loadPlugin( plugin );
                if( arv != 0 )
                {
                    if( !loader.unload() )
                    {
                        std::cerr << "rtimv: unloading an unused plugin failed\n";
                    }
                }
                else
                {
                    std::cerr << "rtimv: loaded plugin " << fileName.toStdString() << "\n";
                    m_pluginFileNames += fileName;
                }
            }
        }
    }

    setWindowTitle( m_title.c_str() );
}

rtimvMainWindow::~rtimvMainWindow()
{
    if( imStats )
    {
        delete imStats;
    }
}

void rtimvMainWindow::setupConfig()
{
    config.add( "image.key",
                "",
                "image.key",
                argType::Required,
                "image",
                "key",
                false,
                "string",
                "The main image key. Specifies the protocol, location, and name of the main image." );

    config.add( "dark.key",
                "",
                "dark.key",
                argType::Required,
                "dark",
                "key",
                false,
                "string",
                "The dark image key. Specifies the protocol, location, and name of the dark image." );

    config.add( "mask.key",
                "",
                "mask.key",
                argType::Required,
                "mask",
                "key",
                false,
                "string",
                "The mask image key. Specifies the protocol, location, and name of the mask image." );

    config.add( "satMask.key",
                "",
                "satMask.key",
                argType::Required,
                "satMask",
                "key",
                false,
                "string",
                "The saturation mask image key. Specifies the protocol, location, "
                "and name of the saturation mask image." );

    config.add( "update.fps",
                "",
                "update.fps",
                argType::Required,
                "update",
                "fps",
                false,
                "real",
                "Specify the image update timeout in FPS.  Overridden by update.timeout if set." );

    config.add( "update.timeout",
                "",
                "update.timeout",
                argType::Required,
                "update",
                "timeout",
                false,
                "real",
                "Specify the image update timeout in ms.  Default is 50 ms (20 FPS). Overrides update.fps." );

    config.add( "update.cubeFPS",
                "",
                "update.cubeFPS",
                argType::Required,
                "update",
                "cubeFPS",
                false,
                "real",
                "Specify the image cube update rate in FPS.  Default is 20 FPS." );

    config.add( "autoscale",
                "",
                "autoscale",
                argType::True,
                "",
                "autoscale",
                false,
                "bool",
                "Set to turn autoscaling on at startup" );

    config.add( "nofpsgage",
                "",
                "nofpsgage",
                argType::True,
                "",
                "nofpsgage",
                false,
                "bool",
                "Set to turn the fps gage off at startup" );

    config.add( "darksub",
                "",
                "darksub",
                argType::True,
                "",
                "darksub",
                false,
                "bool",
                "Set to false to turn off dark subtraction at startup. "
                "If a dark is supplied, darksub is otherwise on." );

    config.add( "targetXc",
                "",
                "targetXc",
                argType::Required,
                "",
                "targetXc",
                false,
                "float",
                "The fractional x-coordinate of the target, 0<= x <=1" );

    config.add( "targetYc",
                "",
                "targetYc",
                argType::Required,
                "",
                "targetYc",
                false,
                "float",
                "The fractional y-coordinate of the target, 0<= y <=1" );

    config.add( "satLevel",
                "",
                "satLevel",
                argType::Required,
                "",
                "satLevel",
                false,
                "float",
                "The saturation level for this camera" );

    config.add( "masksat",
                "",
                "masksat",
                argType::True,
                "",
                "masksat",
                false,
                "bool",
                "Set to false to turn off sat-masking at startup. "
                "If a satMaks is supplied, masksat is otherwise on." );

    config.add( "mouse.pointerCoords",
                "",
                "mouse.pointerCoords",
                argType::Required,
                "mouse",
                "pointerCoords",
                false,
                "bool",
                "Show or don't show the pointer coordinates.  Default is true." );

    config.add( "mouse.staticCoords",
                "",
                "mouse.staticCoords",
                argType::Required,
                "mouse",
                "staticCoords",
                false,
                "bool",
                "Show or don't show the static coordinates at bottom of display.  Default is false." );

    config.add( "mzmq.always",
                "Z",
                "mzmq.always",
                argType::True,
                "mzmq",
                "always",
                false,
                "bool",
                "Set to make milkzmq the protocol for bare image names.  Note that local shmims can"
                "not be used if this is set." );

    config.add( "mzmq.server",
                "s",
                "mzmq.server",
                argType::Required,
                "mzmq",
                "server",
                false,
                "string",
                "The default server for milkzmq.  The default default is localhost.  This will be overridden by an "
                "image specific server specified in a key." );

    config.add( "mzmq.port",
                "p",
                "mzmq.port",
                argType::Required,
                "mzmq",
                "port",
                false,
                "int",
                "The default port for milkzmq.  The default default is 5556.  This will be overridden by an image "
                "specific port specified in a key." );

    config.add( "north.enabled",
                "",
                "north.enabled",
                argType::Required,
                "north",
                "enabled",
                false,
                "bool",
                "Whether or not to enable the north arrow. Default is true." );

    config.add( "north.offset",
                "",
                "north.offset",
                argType::Required,
                "north",
                "offset",
                false,
                "float",
                "Offset in degrees c.c.w. to apply to the north angle. Default is 0." );

    config.add( "north.scale",
                "",
                "north.scale",
                argType::Required,
                "north",
                "scale",
                false,
                "float",
                "Scaling factor to apply to north angle to convert to degrees c.c.w. on the image.  Default is -1." );

    config.add( "tools.lineWidth",
                "",
                "tools.lineWidth",
                argType::Required,
                "tools",
                "lineWidth",
                false,
                "float",
                "The width of lines in user items in screen pixels.  Default is 2." );

    config.add( "tools.edgeTol",
                "",
                "tools.edgeTol",
                argType::Required,
                "tools",
                "edgeTol",
                false,
                "float",
                "The tolerance in screen pixels for the mouse to be on the edge of a user item.  For closed shapes "
                "this applies only to the inside. Default is 5.5 " );

    config.add( "tools.lineHeadRad",
                "",
                "tools.lineHeadRad",
                argType::Required,
                "tools",
                "lineHeadRad",
                false,
                "float",
                "The radius of the circle marking the head of a user line, in screen pixels. Default is 10." );

    config.add( "tools.crossWidthFract",
                "",
                "tools.crossWidthFract",
                argType::Required,
                "tools",
                "crossWidthFract",
                false,
                "float",
                "The half-width of the center cross, relative to the smallest "
                "dimension of the tools. Default is 0.1." );

    config.add( "tools.crossWidthMin",
                "",
                "tools.crossWidthMin",
                argType::Required,
                "tools",
                "crossWidthMin",
                false,
                "float",
                "The minimum half-width of the center cross, in screen pixels. Default is 5." );

    config.add( "tools.warningBorderWidth",
                "",
                "tools.warningBorderWidth",
                argType::Required,
                "tools",
                "warningBorderWidth",
                false,
                "float",
                "The width of the warning border in screen pixels.  Default is 5." );
}

void rtimvMainWindow::loadConfig()
{
    std::string imKey;
    std::string darkKey;

    std::string flatKey;

    std::string maskKey;

    std::string satMaskKey;

    std::vector<std::string> keys;

    // Set up milkzmq
    config( m_mzmqAlways, "mzmq.always" );
    config( m_mzmqServer, "mzmq.server" );
    config( m_mzmqPort, "mzmq.port" );

    // Check for use of deprecated shmim_name keyword by itself, but use key if available
    config( imKey, "image.key" );

    config( darkKey, "dark.key" );

    config( maskKey, "mask.key" );

    config( satMaskKey, "satMask.key" );

    // Populate the key vector, a "" means no image specified
    keys.resize( 4 );

    if( imKey != "" )
        keys[0] = imKey;
    if( darkKey != "" )
        keys[1] = darkKey;
    if( maskKey != "" )
        keys[2] = maskKey;
    if( satMaskKey != "" )
        keys[3] = satMaskKey;

    // The command line always overrides the config
    if( config.nonOptions.size() > 0 )
        keys[0] = config.nonOptions[0];
    if( config.nonOptions.size() > 1 )
        keys[1] = config.nonOptions[1];
    if( config.nonOptions.size() > 2 )
        keys[2] = config.nonOptions[2];
    if( config.nonOptions.size() > 3 )
        keys[3] = config.nonOptions[3];

    startup( keys );

    if( m_images[0] == nullptr )
    {
        if( doHelp )
        {
            help();
        }
        else
        {
            std::cerr << "rtimv: No valid image specified so cowardly refusing to start.  Use -h for help.\n";
        }

        exit( 0 );
    }
    else
    {
        m_title = m_images[0]->imageName();
    }

    // Now load remaining options, respecting coded defaults.

    //get timeouts.
    float fps = -999;
    config( fps, "update.fps" );

    if( fps > 0 ) //fps sets m_imageTimeout
    {
        m_imageTimeout = std::round( 1000. / fps );
    }

    //but update.timeout can override it
    config( m_imageTimeout, "update.timeout" );
    config( m_cubeFPS, "update.cubeFPS" );

    //Now set the actual timeouts
    cubeFPS(m_cubeFPS);

    config( m_autoScale, "autoscale" );
    config( m_subtractDark, "darksub" );

    bool nofpsgage = !m_showFPSGage;
    config( nofpsgage, "nofpsgage" );
    m_showFPSGage = !nofpsgage;

    config( m_targetXc, "targetXc" );
    config( m_targetYc, "targetYc" );

    float satLevelDefault = m_satLevel;
    config( m_satLevel, "satLevel" );

    // If we set a sat level or mask, apply it
    if( m_satLevel != satLevelDefault || satMaskKey != "" )
    {
        m_applySatMask = true;
    }

    // except turn it off if requested
    config( m_applySatMask, "masksat" );

    config( m_showToolTipCoords, "mouse.pointerCoords" );
    config( m_showStaticCoords, "mouse.staticCoords" );

    config( m_northArrowEnabled, "north.enabled" );
    config( m_northAngleOffset, "north.offset" );
    config( m_northAngleScale, "north.scale" );

    config( m_userItemLineWidth, "tools.lineWidth" );
    config( m_userItemEdgeTol, "tools.edgeTol" );
    config( m_userLineHeadRad, "tools.lineHeadRad" );
    config( m_userItemCrossWidthFract, "tools.crossWidthFract" );
    config( m_userItemCrossWidthMin, "tools.crossWidthMin" );
    config( m_warningBorderWidth, "tools.warningBorderWidth" );
}

void rtimvMainWindow::onConnect()
{
    setWindowTitle( m_title.c_str() );

    squareDown();

    if( nz() > 1 && !m_cubeCtrl )
    {
        launchCubeCtrl();
    }

    m_connected = true;
}

void rtimvMainWindow::mtxL_postSetImsize( const uniqueLockT &lock )
{
    assert( lock.owns_lock() );

    m_screenZoom = std::min( (float)ui.graphicsView->viewport()->width() / (float)m_nx,
                             (float)ui.graphicsView->viewport()->height() / (float)m_ny );

    // We're mutexed when we get here
    if( m_qpmi )
    {
        delete m_qpmi;
        m_qpmi = nullptr;
    }

    if( imcp )
    {
        QTransform transform;
        float viewZoom = (float)imcp->ui.viewView->width() / (float)m_nx;

        transform.scale( viewZoom, viewZoom );
        imcp->ui.viewView->setTransform( transform );
    }

    statsBoxRemove( m_statsBox );

    colorBoxRemove( m_colorBox );

    auto ubit = m_userBoxes.begin();
    while( ubit != m_userBoxes.end() )
    {
        StretchBox *sb = *ubit;
        userBoxRemove( sb );
        ubit = m_userBoxes.begin();
    }

    auto ucit = m_userCircles.begin();
    while( ucit != m_userCircles.end() )
    {
        StretchCircle *sc = *ucit;
        userCircleRemove( sc );
        ucit = m_userCircles.begin();
    }

    auto ulit = m_userLines.begin();
    while( ulit != m_userLines.end() )
    {
        StretchLine *sl = *ulit;
        userLineRemove( sl );
        ulit = m_userLines.begin();
    }

    setTarget();
}

void rtimvMainWindow::post_zoomLevel()
{
    RTIMV_DEBUG_BREADCRUMB
    QTransform transform;

    ui.graphicsView->screenZoom( m_screenZoom );

    transform.scale( m_zoomLevel * m_screenZoom, m_zoomLevel * m_screenZoom );

    ui.graphicsView->setTransform( transform );

    if( imcp )
    {
        transform.scale( pointerOverZoom, pointerOverZoom );
        imcp->ui.pointerView->setTransform( transform );
    }

    RTIMV_DEBUG_BREADCRUMB

    change_center();

    RTIMV_DEBUG_BREADCRUMB

    if( m_statsBox )
    {
        m_statsBox->setPenWidth( m_userItemLineWidth / m_screenZoom / m_zoomLevel );
        m_statsBox->setEdgeTol( m_userItemEdgeTol / m_screenZoom / m_zoomLevel );
    }

    if( m_colorBox )
    {
        m_colorBox->setPenWidth( m_userItemLineWidth / m_screenZoom / m_zoomLevel );
        m_colorBox->setEdgeTol( m_userItemEdgeTol / m_screenZoom / m_zoomLevel );
    }

    for( auto &ubit : m_userBoxes )
    {
        ubit->setPenWidth( m_userItemLineWidth / m_screenZoom / m_zoomLevel );
        ubit->setEdgeTol( m_userItemEdgeTol / m_screenZoom / m_zoomLevel );
    }

    for( auto &ucit : m_userCircles )
    {
        ucit->setPenWidth( m_userItemLineWidth / m_screenZoom / m_zoomLevel );
        ucit->setEdgeTol( m_userItemEdgeTol / m_screenZoom / m_zoomLevel );
    }

    for( auto &ulit : m_userLines )
    {
        ulit->setPenWidth( m_userItemLineWidth / m_screenZoom / m_zoomLevel );
        ulit->setEdgeTol( m_userItemEdgeTol / m_screenZoom / m_zoomLevel );
    }

    RTIMV_DEBUG_BREADCRUMB

    setBorderBox();

    RTIMV_DEBUG_BREADCRUMB

    char zlstr[16];
    snprintf( zlstr, 16, "%0.1fx", m_zoomLevel );
    ui.graphicsView->zoomText( zlstr );

    RTIMV_DEBUG_BREADCRUMB

    mtxTry_fontLuminance( ui.graphicsView->zoomText() );

    RTIMV_DEBUG_BREADCRUMB
}

void rtimvMainWindow::mtxL_postRecolor( const uniqueLockT &lock )
{
    assert( lock.owns_lock() );

    RTIMV_DEBUG_BREADCRUMB

    if( !m_qpmi ) // This happens on first time through
    {
        RTIMV_DEBUG_BREADCRUMB

        m_qpmi = m_qgs->addPixmap( m_qpm );

        // So we need to initialize the viewport center, etc.
        // center();
        mtxL_setViewCen( .5, .5, lock );
        post_zoomLevel();

        // and update stats box
        if( m_statsBox )
        {
            mtxTry_statsBoxMoved( m_statsBox );
        }

        RTIMV_DEBUG_BREADCRUMB
    }
    else
    {
        m_qpmi->setPixmap( m_qpm );
        RTIMV_DEBUG_BREADCRUMB
    }

    RTIMV_DEBUG_BREADCRUMB
}

void rtimvMainWindow::mtxL_postRecolor( const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    RTIMV_DEBUG_BREADCRUMB

    if( !m_qpmi ) // This happens on first time through
    {
        RTIMV_DEBUG_BREADCRUMB

        m_qpmi = m_qgs->addPixmap( m_qpm );

        // So we need to initialize the viewport center, etc.
        // center();
        mtxL_setViewCen( .5, .5, lock );
        post_zoomLevel();

        // and update stats box
        if( m_statsBox )
        {
            mtxTry_statsBoxMoved( m_statsBox );
        }

        RTIMV_DEBUG_BREADCRUMB
    }
    else
    {
        m_qpmi->setPixmap( m_qpm );
        RTIMV_DEBUG_BREADCRUMB
    }

    RTIMV_DEBUG_BREADCRUMB
}

void rtimvMainWindow::mtxL_postChangeImdata( const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    RTIMV_DEBUG_BREADCRUMB

    // We're mutexed when we get here
    if( m_saturated && !m_applySatMask )
    {
        ui.graphicsView->warningText( "Saturated!" );
    }
    else
    {
        ui.graphicsView->warningText( "" );
    }

    RTIMV_DEBUG_BREADCRUMB

    if( m_borderBox )
        m_qpmi->stackBefore( m_borderBox );
    if( m_colorBox )
        m_qpmi->stackBefore( m_colorBox );
    if( m_statsBox )
        m_qpmi->stackBefore( m_statsBox );
    if( m_objCenH )
        m_qpmi->stackBefore( m_objCenH );
    if( m_objCenV )
        m_qpmi->stackBefore( m_objCenV );
    if( m_lineHead )
        m_qpmi->stackBefore( m_lineHead );
    if( m_northArrow )
        m_qpmi->stackBefore( m_northArrow );
    if( m_northArrowTip )
        m_qpmi->stackBefore( m_northArrowTip );

    RTIMV_DEBUG_BREADCRUMB

    if( imcp )
    {
        if( imcp->ViewViewMode == ViewViewEnabled )
        {
            if( !imcp->qpmi_view )
                imcp->qpmi_view = imcp->qgs_view->addPixmap( m_qpm );
            imcp->qpmi_view->setPixmap( m_qpm );

            imcp->qpmi_view->stackBefore( imcp->viewLineVert );
        }
    }

    RTIMV_DEBUG_BREADCRUMB

    mtxL_updateMouseCoords( lock ); // This is to update the pixel val box if set.

    RTIMV_DEBUG_BREADCRUMB

    if( imcp )
    {
        imcp->update_panel();
    }

    RTIMV_DEBUG_BREADCRUMB

    if( imStats )
    {
        imStats->setImdata();
    }

    RTIMV_DEBUG_BREADCRUMB

    if( m_connected ) // first time through this won't be true
    {
        if( m_images[0] != nullptr ) // really can't be true if connected
        {
            if( m_images[0]->nz() > 1 && m_cubeCtrl == nullptr ) // new image is cube for first time
            {
                cubeMode( true );
                launchCubeCtrl();
            }
        }
    }
}

void rtimvMainWindow::launchControlPanel()
{
    if( !imcp )
    {
        imcp = new rtimvControlPanel( this, &m_calMutex, Qt::Tool );
        connect( imcp, SIGNAL( launchStatsBox() ), this, SLOT( doLaunchStatsBox() ) );
        connect( imcp, SIGNAL( hideStatsBox() ), this, SLOT( doHideStatsBox() ) );
    }

    imcp->show();

    imcp->activateWindow();
}

void rtimvMainWindow::launchCubeCtrl()
{
    if( m_cubeCtrl == nullptr )
    {
        int fno = 0;
        if( m_images[0] != nullptr )
        {
            fno = m_images[0]->imageNo();
        }

        m_cubeCtrl = new cubeCtrl( m_cubeMode,
                                   m_cubeFPS,
                                   m_desiredCubeFPS,
                                   m_cubeFPSMult,
                                   m_cubeDir,
                                   nz(),
                                   fno,
                                   m_autoScale,
                                   this,
                                   Qt::Dialog );

        connect( this, SIGNAL( cubeModeUpdated( bool ) ), m_cubeCtrl, SLOT( cubeMode( bool ) ) );
        connect( m_cubeCtrl, SIGNAL( cubeModeUpdated( bool ) ), this, SLOT( cubeMode( bool ) ) );

        connect( this, SIGNAL( cubeFPSUpdated( float, float ) ), m_cubeCtrl, SLOT( cubeFPS( float, float ) ) );
        connect( m_cubeCtrl, SIGNAL( cubeFPSUpdated( float ) ), this, SLOT( cubeFPS( float ) ) );

        connect( this, SIGNAL( cubeFPSMultUpdated( float ) ), m_cubeCtrl, SLOT( cubeFPSMult( float ) ) );
        connect( m_cubeCtrl, SIGNAL( cubeFPSMultUpdated( float ) ), this, SLOT( cubeFPSMult( float ) ) );

        connect( this, SIGNAL( cubeDirUpdated( int ) ), m_cubeCtrl, SLOT( cubeDir( int ) ) );
        connect( m_cubeCtrl, SIGNAL( cubeDirUpdated( int ) ), this, SLOT( cubeDir( int ) ) );

        connect( this, SIGNAL( nzUpdated( uint32_t ) ), m_cubeCtrl, SLOT( cubeFrames( uint32_t ) ) );

        connect( this, SIGNAL( cubeFrameUpdated( uint32_t ) ), m_cubeCtrl, SLOT( cubeFrame( uint32_t ) ) );
        connect( m_cubeCtrl, SIGNAL( cubeFrameUpdated( uint32_t ) ), this, SLOT( cubeFrame( uint32_t ) ) );

        connect( m_cubeCtrl, SIGNAL( cubeFrameDeltaUpdated( int32_t ) ), this, SLOT( cubeFrameDelta( int32_t ) ) );

        connect( this, SIGNAL( autoScaleUpdated( bool ) ), m_cubeCtrl, SLOT( autoScale( bool ) ) );
        connect( m_cubeCtrl, SIGNAL( autoScaleUpdated( bool ) ), this, SLOT( autoScale( bool ) ) );

        connect( m_cubeCtrl->restretchButton(), SIGNAL( pressed() ), this, SLOT( reStretch() ) );

        // Position cubeCtrl below window, centered, with minimum size.
        QRect mw = this->geometry();
        QRect cc = m_cubeCtrl->geometry();
        // This minimizes the width
        m_cubeCtrl->setGeometry( cc.x(), cc.y(), 0.5 * cc.width(), cc.height() );
        cc = m_cubeCtrl->geometry();

        int tbh = QApplication::style()->pixelMetric( QStyle::PM_TitleBarHeight );
        m_cubeCtrl->setGeometry(
            mw.x() - ( cc.width() - mw.width() ), mw.y() + height() + 2 * tbh, cc.width(), cc.height() );

        m_cubeCtrl->setWindowTitle( QString( "cube: %1" ).arg( m_title.c_str() ) );
    }

    m_cubeCtrl->show();
}

void rtimvMainWindow::toggleCubeCtrl()
{
    if( m_cubeCtrl == nullptr )
    {
        launchCubeCtrl();
    }
    else
    {
        if( m_cubeCtrl->isVisible() )
        {
            m_cubeCtrl->hide();
        }
        else
        {
            m_cubeCtrl->show();
        }
    }
}

void rtimvMainWindow::centerNorthArrow()
{
    if( m_northArrow && m_northArrowTip )
    {
        m_northArrow->setLine( ui.graphicsView->xCen(),
                               ui.graphicsView->yCen() - .1 * m_ny / m_zoomLevel,
                               ui.graphicsView->xCen(),
                               ui.graphicsView->yCen() + .1 * m_ny / m_zoomLevel );

        m_northArrow->setTransformOriginPoint( QPointF( ui.graphicsView->xCen(), ui.graphicsView->yCen() ) );

        m_northArrowTip->setLine(
            QLineF( ui.graphicsView->xCen(),
                    ui.graphicsView->yCen() - .1 * m_ny / m_zoomLevel,
                    ui.graphicsView->xCen() + .02 * m_nx / m_zoomLevel,
                    ui.graphicsView->yCen() - .1 * m_ny / m_zoomLevel + .012 * m_ny / m_zoomLevel ) );
        m_northArrowTip->setTransformOriginPoint( QPointF( ui.graphicsView->xCen(), ui.graphicsView->yCen() ) );

        QPen qp = m_northArrow->pen();

        float wid = 5 / ( m_zoomLevel * m_screenZoom );
        if( wid > 3 )
            wid = 3;
        qp.setWidth( wid );

        m_northArrow->setPen( qp );
        m_northArrowTip->setPen( qp );
    }

    updateNorthArrow();
}

void rtimvMainWindow::updateNorthArrow()
{
    if( m_northArrow && m_northArrowTip )
    {
        float ang = northAngle();
        m_northArrow->setRotation( ang );
        m_northArrowTip->setRotation( ang );
    }
}

float rtimvMainWindow::northAngle()
{
    float north = 0;
    if( m_dictionary.count( "rtimv.north.angle" ) > 0 )
    {
        m_dictionary["rtimv.north.angle"].getBlob( (char *)&north, sizeof( float ) );

        north = -1 * ( m_northAngleOffset + m_northAngleScale * north ); // negative because QT is c.w.
    }

    return north;
}

void rtimvMainWindow::northAngleRaw( float north )
{
    m_dictionary["rtimv.north.angle"].setBlob( (char *)&north, sizeof( float ) );
}

QGraphicsScene *rtimvMainWindow::get_qgs()
{
    return m_qgs;
}

void rtimvMainWindow::freezeRealTime()
{
    if( RealTimeStopped )
    {
        set_RealTimeStopped( false );
    }
    else
    {
        set_RealTimeStopped( true );
        if( m_showFPSGage )
            ui.graphicsView->fpsGageText( 0.0 );
    }
}

void rtimvMainWindow::reStretch()
{
    if( get_colorbar_mode() == user )
    {
        set_colorbar_mode( minmaxglobal );
    }
    else if( get_colorbar_mode() == minmaxglobal )
    {
        mindat( get_imdat_min() );
        maxdat( get_imdat_max() );
    }
    else if( get_colorbar_mode() == minmaxbox )
    {
        mindat( m_colorBox_min );
        maxdat( m_colorBox_max );
    }

    sharedLockT lock( m_calMutex );
    mtxL_recolor( lock );
}

void rtimvMainWindow::setPointerOverZoom( float poz )
{
    pointerOverZoom = poz;
    post_zoomLevel();
}

void rtimvMainWindow::change_center( bool movezoombox )
{
    ui.graphicsView->centerOn( (qreal)ui.graphicsView->xCen(), (qreal)ui.graphicsView->yCen() );

    centerNorthArrow();

    if( imcp )
    {
        imcp->viewLineVert->setLine( ui.graphicsView->xCen(), 0, ui.graphicsView->xCen(), m_ny );
        imcp->viewLineHorz->setLine( 0, ui.graphicsView->yCen(), m_nx, ui.graphicsView->yCen() );

        if( m_zoomLevel <= 1.0 )
            imcp->viewBox->setVisible( false );
        else
        {
            imcp->viewBox->setVisible( true );
            if( movezoombox )
            {
                QPointF tmpp = imcp->viewBox->mapFromParent( ui.graphicsView->xCen() - .5 * m_nx / m_zoomLevel,
                                                             ui.graphicsView->yCen() - .5 * m_ny / m_zoomLevel );
                imcp->viewBox->setRect( tmpp.x(), tmpp.y(), m_nx / m_zoomLevel, m_ny / m_zoomLevel );
            }
        }
        imcp->ui.viewView->centerOn( .5 * m_nx, .5 * m_ny );
        imcp->update_panel();
    }
}

void rtimvMainWindow::mtxL_setViewCen_impl( float x, float y, bool movezoombox )
{
    if( m_qpmi == nullptr )
    {
        return;
    }

    QPointF sp( x * m_qpmi->boundingRect().width(), y * m_qpmi->boundingRect().height() );

    QPointF vp = ui.graphicsView->mapFromScene( sp );

    ui.graphicsView->mapCenterToScene( vp.x(), vp.y() );

    change_center( movezoombox );
}

void rtimvMainWindow::mtxL_setViewCen( float x, float y, const uniqueLockT &lock, bool movezoombox )
{
    assert( lock.owns_lock() );

    mtxL_setViewCen_impl( x, y, movezoombox );
}

void rtimvMainWindow::mtxL_setViewCen( float x, float y, const sharedLockT &lock, bool movezoombox )
{
    assert( lock.owns_lock() );

    mtxL_setViewCen_impl( x, y, movezoombox );
}

void rtimvMainWindow::squareDown()
{
    double imrat = ( (double)nx() ) / ny();
    double winrat = ( (double)width() ) / height();

    ///\todo make threshold responsive to current dimensions so we don't enter loops.
    if( fabs( 1.0 - imrat / winrat ) < 0.01 )
    {
        return;
    }

    if( width() <= height() )
    {
        resize( width(), width() / imrat );
    }
    else
    {
        resize( height() * imrat, height() );
    }
}

void rtimvMainWindow::squareUp()
{
    double imrat = ( 1.0 * nx() ) / ny();
    double winrat = ( 1.0 * width() ) / height();

    ///\todo make threshold responsive to current dimensions so we don't enter loops.
    if( fabs( 1.0 - imrat / winrat ) < 0.01 )
    {
        return;
    }

    if( width() <= height() )
    {
        resize( height() * imrat, height() );
    }
    else
    {
        resize( width(), width() / imrat );
    }
}

void rtimvMainWindow::resizeEvent( QResizeEvent * )
{
    if( m_nx == 0 || m_ny == 0 )
    {
        ui.graphicsView->setGeometry( 0, 0, width(), height() );
        return;
    }

    m_screenZoom = std::min( (float)width() / (float)m_nx, (float)height() / (float)m_ny );

    change_center();

    ui.graphicsView->setGeometry( 0, 0, width(), height() );

    post_zoomLevel();
}

void rtimvMainWindow::mouseMoveEvent( QMouseEvent *e )
{
    nullMouseCoords();
    (void)( e );
}

void rtimvMainWindow::nullMouseCoords()
{
    if( !m_nullMouseCoords )
    {
        if( imcp )
        {
            imcp->nullMouseCoords();
        }

        m_nullMouseCoords = true;

        ui.graphicsView->textCoordX( "" );
        ui.graphicsView->textCoordY( "" );
        ui.graphicsView->textPixelVal( "" );

        ui.graphicsView->hideMouseToolTip();
    }
}

void rtimvMainWindow::mtxL_updateMouseCoords( const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    int64_t idx_x, idx_y; // image size are uint32_t, so this allows signed comparison without overflow issues

    if( !m_qpmi )
        return;

    if( ui.graphicsView->mouseViewX() < 0 || ui.graphicsView->mouseViewY() < 0 )
    {
        nullMouseCoords();
    }

    QPointF pt = ui.graphicsView->mapToScene( ui.graphicsView->mouseViewX(), ui.graphicsView->mouseViewY() );

    float mx = pt.x();
    float my = pt.y();

    if( mx < 0 || mx > m_qpmi->boundingRect().width() || my < 0 || my > m_qpmi->boundingRect().height() )
    {
        nullMouseCoords();
    }

    if( m_userItemSelected )
    {
        nullMouseCoords();
        mtxTry_userItemMouseCoords( m_userItemMouseViewX, m_userItemMouseViewY, m_userItemXCen, m_userItemYCen );
    }

    if( !m_nullMouseCoords )
    {
        idx_x = ( (int64_t)( mx - 0 ) );
        if( idx_x < 0 )
            idx_x = 0;
        if( idx_x > (int64_t)m_nx - 1 )
            idx_x = m_nx - 1;

        idx_y = (int)( m_qpmi->boundingRect().height() - ( my - 0 ) );
        if( idx_y < 0 )
            idx_y = 0;
        if( idx_y > (int64_t)m_ny - 1 )
            idx_y = m_ny - 1;

        float val;

        val = calPixel( idx_x, idx_y );

        if( m_showStaticCoords )
        {
            ui.graphicsView->textCoordX( mx - 0.5 );
            ui.graphicsView->textCoordY( m_qpmi->boundingRect().height() - my - 0.5 );
            ui.graphicsView->textPixelVal( val );
        }

        if( m_showToolTipCoords )
        {
            char valStr[32];
            char posStr[32];

            if( fabs( val ) < 1e-1 )
            {
                snprintf( valStr, sizeof( valStr ), "%0.04g", val );
            }
            else
            {
                snprintf( valStr, sizeof( valStr ), "%0.02f", val );
            }
            snprintf( posStr, sizeof( posStr ), "%0.2f %0.2f", mx - 0.5, m_qpmi->boundingRect().height() - my - 0.5 );
            ui.graphicsView->showMouseToolTip(
                valStr, posStr, QPoint( ui.graphicsView->mouseViewX(), ui.graphicsView->mouseViewY() ) );
            mtxTry_fontLuminance( ui.graphicsView->mouseCoords() );
        }

        if( imcp )
        {
            imcp->updateMouseCoords( mx, my, val );
        }
    }

    // Adjust bias and contrast
    if( rightClickDragging )
    {
        float dx = ui.graphicsView->mouseViewX() - rightClickStart.x();
        float dy = ui.graphicsView->mouseViewY() - rightClickStart.y();

        float dbias = dx / ui.graphicsView->viewport()->width();
        float dcontrast = -1. * dy / ui.graphicsView->viewport()->height();

        bias( biasStart + dbias * .5 * ( imdat_max + imdat_min ) );
        contrast( contrastStart + dcontrast * ( imdat_max - imdat_min ) );
        if( !m_amChangingimdata )
            mtxL_recolor( lock );
    }

} // rtimvMainWindow::mtxL_updateMouseCoords

bool rtimvMainWindow::showToolTipCoords()
{
    return m_showToolTipCoords;
}

bool rtimvMainWindow::showStaticCoords()
{
    return m_showStaticCoords;
}

void rtimvMainWindow::showToolTipCoords( bool sttc )
{
    m_showToolTipCoords = sttc;
    emit showToolTipCoordsChanged( m_showToolTipCoords );
}

void rtimvMainWindow::showStaticCoords( bool ssc )
{
    m_showStaticCoords = ssc;
    emit showStaticCoordsChanged( m_showStaticCoords );
}

void rtimvMainWindow::changeMouseCoords()
{
    m_nullMouseCoords = false;

    sharedLockT lock( m_calMutex );
    mtxL_updateMouseCoords( lock );
}

void rtimvMainWindow::viewLeftPressed( QPointF mp )
{
    if( imcp )
    {
        imcp->viewLeftPressed( mp );
    }
}

void rtimvMainWindow::viewLeftClicked( QPointF mp )
{
    if( imcp )
    {
        imcp->viewLeftClicked( mp );
    }
}

void rtimvMainWindow::viewRightPressed( QPointF mp )
{
    rightClickDragging = true;

    rightClickStart = mp; // ui.graphicsView->mapToScene(mp.x(),mp.y());
    biasStart = bias();
    contrastStart = contrast();
}

void rtimvMainWindow::viewRightClicked( QPointF mp )
{
    rightClickDragging = false;
    (void)( mp );
}

void rtimvMainWindow::onWheelMoved( int delta )
{
    float dz;
    if( delta > 0 )
        dz = 1.02; // 1.41421;
    else
        dz = 0.98; // 0.70711;

    zoomLevel( dz * zoomLevel() );
}

void rtimvMainWindow::updateFPS()
{
    updateAge();
}

void rtimvMainWindow::updateAge()
{
    // Check the font luminance to make sure it is visible
    mtxTry_fontLuminance();

    if( m_showFPSGage && m_images[0] != nullptr )
    {
        struct timespec tstmp;

        clock_gettime( CLOCK_REALTIME, &tstmp );

        double timetmp = (double)tstmp.tv_sec + ( (double)tstmp.tv_nsec ) / 1e9;

        double fpsTime = m_images[0]->imageTime();

        double age = timetmp - fpsTime;

        if( m_images[0]->fpsEst() > 1.0 && age < 2.0 )
        {
            ui.graphicsView->fpsGageText( m_images[0]->fpsEst() );
        }
        else if( age < 86400 * 10000 ) // only if age is reasonable
        {
            ui.graphicsView->fpsGageText( age, true );
        }
        else
        {
            ui.graphicsView->fpsGageText( "" );
        }
    }

    for( size_t n = 0; n < m_overlays.size(); ++n )
    {
        m_overlays[n]->updateOverlay();
    }

    if( m_showLoopStat )
    {
        ui.graphicsView->loopText( "Loop OPEN", "red" );
    }

    if( m_northArrowEnabled )
    {
        updateNorthArrow();
    }
}

void rtimvMainWindow::updateNC()
{
    for( size_t n = 0; n < m_overlays.size(); ++n )
    {
        m_overlays[n]->updateOverlay();
    }
}

void rtimvMainWindow::mtxTry_userItemMouseCoords( float mx, float my, float dx, float dy )
{
    sharedLockT lock( m_calMutex );

    if( m_qpmi == nullptr )
        return;

    int idx_x = ( (int64_t)( mx - 0 ) );
    if( idx_x < 0 )
        idx_x = 0;
    if( idx_x > (int64_t)m_nx - 1 )
        idx_x = m_nx - 1;

    int idx_y = (int)( m_qpmi->boundingRect().height() - ( my - 0 ) );

    if( idx_y < 0 )
        idx_y = 0;
    if( idx_y > (int64_t)m_ny - 1 )
        idx_y = m_ny - 1;

    float val;
    val = calPixel( idx_x, idx_y );

    char valStr[32];
    char posStr[64];

    if( fabs( val ) < 1e-1 )
    {
        snprintf( valStr, sizeof( valStr ), "%0.04g", val );
    }
    else
    {
        snprintf( valStr, sizeof( valStr ), "%0.02f", val );
    }

    snprintf(
        posStr, sizeof( posStr ), "%s\n%0.2f %0.2f", valStr, mx - 0.5, m_qpmi->boundingRect().height() - my - 0.5 );

    QFontMetrics fm( ui.graphicsView->userItemMouseCoords()->currentFont() );
    QSize textSize = fm.size( 0, posStr );

    float offsetx = 0;
    float offsety = 0;
    if( m_offsetItemMouseCoordsX )
    {
        offsetx = textSize.width() + ( m_userLineHeadRad );
    }

    if( m_offsetItemMouseCoordsY )
    {
        offsety = textSize.height() + ( m_userLineHeadRad );
    }

    // Take scene coordinates to viewport coordinates.
    QPoint qr = ui.graphicsView->mapFromScene( QPointF( dx, dy ) );

    QRect rect( qr.x() - offsetx, qr.y() - offsety, textSize.width() + 5, textSize.height() + 5 );

    ui.graphicsView->userItemMouseCoordsText( posStr, rect );

    mtxL_fontLuminance( ui.graphicsView->userItemMouseCoords(), lock );
}

void rtimvMainWindow::userItemSelected( const QColor &color, bool sizeVis, bool coordsVis, bool cenVis )
{
    ui.graphicsView->userItemColor( color.name() );

    ui.graphicsView->userItemSizeText( " " );
    ui.graphicsView->userItemSize()->setVisible( sizeVis );

    ui.graphicsView->userItemMouseCoordsText( " " );
    ui.graphicsView->userItemMouseCoords()->setVisible( coordsVis );

    if( cenVis )
    {
        QPen pH = m_objCenH->pen();
        pH.setColor( color );
        m_objCenH->setPen( pH );
        m_objCenH->setVisible( cenVis );

        QPen pV = m_objCenH->pen();
        pV.setColor( color );
        m_objCenV->setPen( pV );
        m_objCenV->setVisible( cenVis );
    }
    else
    {
        m_objCenH->setVisible( cenVis );
        m_objCenV->setVisible( cenVis );
    }

    nullMouseCoords();

    m_userItemSelected = true;
}

void rtimvMainWindow::userItemCross( const QPointF &pos, const QRectF &rect, const QPen &pen )
{
    m_userItemXCen = rect.x() + pos.x() + 0.5 * rect.width();
    m_userItemYCen = rect.y() + pos.y() + 0.5 * rect.height();

    m_objCenH->setPen( pen );
    m_objCenV->setPen( pen );

    float iw = std::min( rect.width(), rect.height() );

    float cw = iw * m_userItemCrossWidthFract;

    if( cw < m_userItemCrossWidthMin / m_screenZoom )
        cw = m_userItemCrossWidthMin / m_screenZoom;

    if( cw > 0.5 * iw )
        cw = 0.5 * iw;

    m_objCenH->setLine( m_userItemXCen - cw, m_userItemYCen, m_userItemXCen + cw, m_userItemYCen );
    m_objCenV->setLine( m_userItemXCen, m_userItemYCen - cw, m_userItemXCen, m_userItemYCen + cw );
}

/*---- Color Box ----*/

void rtimvMainWindow::mtxTry_colorBoxMoved( StretchBox *sb )
{
    sharedLockT lock( m_calMutex );

    if( !m_colorBox )
        return;
    if( !m_qpmi )
        return;

    if( !colorBoxActive )
        return;

    QRectF newr = sb->rect();

    QPointF np = m_qpmi->mapFromItem( m_colorBox, QPointF( newr.x(), newr.y() ) );
    QPointF np2 = m_qpmi->mapFromItem( m_colorBox, QPointF( newr.x() + newr.width(), newr.y() + newr.height() ) );

    colorBox_i0( (int64_t)( np2.x() + .5 ) );
    colorBox_i1( (int64_t)np.x() );
    colorBox_j0( (int64_t)m_ny - (int64_t)( np2.y() + .5 ) );
    colorBox_j1( (int64_t)m_ny - (int64_t)np.y() );

    char tmp[256];
    char valMin[64];
    char valMax[64];
    if( fabs( m_colorBox_min ) < 1e-1 )
    {
        snprintf( valMin, sizeof( valMin ), "%0.04g", m_colorBox_min );
    }
    else
    {
        snprintf( valMin, sizeof( valMin ), "%0.02f", m_colorBox_min );
    }

    if( fabs( m_colorBox_max ) < 1e-1 )
    {
        snprintf( valMax, sizeof( valMax ), "%0.04g", m_colorBox_max );
    }
    else
    {
        snprintf( valMax, sizeof( valMax ), "%0.02f", m_colorBox_max );
    }

    snprintf( tmp, 256, "min: %s\nmax: %s", valMin, valMax );

    QRectF sbr = sb->sceneBoundingRect();
    QPoint qr = ui.graphicsView->mapFromScene( QPointF( sbr.x(), sbr.y() ) );

    ui.graphicsView->userItemSizeText( tmp, qr );

    mtxL_fontLuminance( ui.graphicsView->userItemSize(), lock );

    mtxL_setColorBoxActive( true, lock ); // recalcs and recolors.
}

void rtimvMainWindow::mtxTry_colorBoxSelected( StretchBox *sb )
{
    userItemSelected( RTIMV_DEF_COLORBOXCOLOR, true, false, false );

    mtxTry_colorBoxMoved( sb );
}

void rtimvMainWindow::colorBoxDeselected( StretchBox *sb )
{
    static_cast<void>( sb );
    ui.graphicsView->userItemSizeText( "" );
    ui.graphicsView->userItemSize()->setVisible( false );
    m_nullMouseCoords = false;
    m_userItemSelected = false;
}

void rtimvMainWindow::colorBoxRemove( StretchBox *sb )
{
    static_cast<void>( sb );

    if( sb == nullptr || m_colorBox == nullptr )
    {
        return;
    }

    colorBoxActive = false;
    set_colorbar_mode( minmaxglobal );

    m_colorBox->disconnect();
    disconnect( m_colorBox );
    m_colorBox->deleteLater();
    m_colorBox = nullptr;

    ui.graphicsView->userItemSize()->setVisible( false );
    ui.graphicsView->userItemMouseCoords()->setVisible( false );
    m_objCenH->setVisible( false );
    m_objCenV->setVisible( false );
    m_nullMouseCoords = false;
    m_userItemSelected = false;
}

/*---- Stats Box and Display ----*/

void rtimvMainWindow::doLaunchStatsBox()
{
    if( !m_statsBox )
        return;

    m_statsBox->setVisible( true );

    if( !imStats )
    {
        imStats = new rtimvStats( this, &m_calMutex, this, Qt::WindowFlags() );
        imStats->setAttribute( Qt::WA_DeleteOnClose ); // Qt will delete imstats when it closes.
        connect( imStats, SIGNAL( finished( int ) ), this, SLOT( imStatsClosed( int ) ) );
    }

    mtxTry_statsBoxMoved( m_statsBox );

    imStats->show();

    imStats->activateWindow();
}

void rtimvMainWindow::doHideStatsBox()
{
    if( imStats )
    {
        delete imStats;
        imStats = 0; // imStats is set to delete on close
    }

    if( m_statsBox )
    {
        if( m_statsBox->isSelected() )
        {
            m_statsBox->setSelected( false );
            userBoxDeSelected( m_statsBox );
        }
        m_statsBox->setVisible( false );
    }
}

void rtimvMainWindow::imStatsClosed( int result )
{
    static_cast<void>( result );

    if( !m_statsBox )
        return;

    m_statsBox->setVisible( false );
    imStats = 0; // imStats is set to delete on close

    doHideStatsBox();

    if( imcp )
    {
        imcp->statsBoxButtonState = false;
        imcp->ui.statsBoxButton->setText( "Show Stats Box" );
    }
}

void rtimvMainWindow::mtxTry_statsBoxMoved( StretchBox *sb )
{
    sharedLockT lock( m_calMutex );

    if( !m_statsBox )
        return;

    if( !m_qpmi )
        return;

    QPointF np = m_qpmi->mapFromItem( m_statsBox, QPointF( m_statsBox->rect().x(), m_statsBox->rect().y() ) );
    QPointF np2 = m_qpmi->mapFromItem( m_statsBox,
                                       QPointF( m_statsBox->rect().x() + m_statsBox->rect().width(),
                                                m_statsBox->rect().y() + m_statsBox->rect().height() ) );

    if( imStats )
    {
        imStats->mtxL_setImdata( m_nx, m_ny, np.x(), np2.x(), m_ny - np2.y(), m_ny - np.y(), lock );
    }

    mtxTry_userBoxMoved( sb );
}

void rtimvMainWindow::mtxTry_statsBoxSelected( StretchBox *sb )
{
    userItemSelected( RTIMV_DEF_STATSBOXCOLOR, true, true, true );

    mtxTry_statsBoxMoved( sb );
}

void rtimvMainWindow::statsBoxRemove( StretchBox *sb )
{
    static_cast<void>( sb );

    if( sb == nullptr || m_statsBox == nullptr )
    {
        return;
    }

    doHideStatsBox();
    m_statsBox->disconnect();
    disconnect( m_statsBox );
    m_statsBox->deleteLater();

    m_statsBox = nullptr;

    ui.graphicsView->userItemSize()->setVisible( false );
    ui.graphicsView->userItemMouseCoords()->setVisible( false );
    m_objCenH->setVisible( false );
    m_objCenV->setVisible( false );
    m_nullMouseCoords = false;
    m_userItemSelected = false;
}

/*---- User Boxes ----*/

void rtimvMainWindow::addUserBox()
{
    float w;

    float znx = m_nx / m_zoomLevel;
    float zny = m_ny / m_zoomLevel;
    if( znx < zny )
        w = znx / 4;
    else
        w = zny / 4;

    std::pair<std::unordered_set<StretchBox *>::iterator, bool> it =
        m_userBoxes.insert( new StretchBox( ui.graphicsView->xCen() - w / 2, ui.graphicsView->yCen() - w / 2, w, w ) );

    StretchBox *sb = *it.first;

    sb->setPenColor( "lime" );
    sb->setPenWidth( m_userItemLineWidth / m_screenZoom / m_zoomLevel );

    sb->setEdgeTol( m_userItemEdgeTol / m_screenZoom / m_zoomLevel );

    sb->setMaintainCenter( true );
    sb->setStretchable( true );
    sb->setVisible( true );
    sb->setRemovable( true );

    connect( sb, SIGNAL( resized( StretchBox * ) ), this, SLOT( mtxTry_userBoxMoved( StretchBox * ) ) );
    connect( sb, SIGNAL( moved( StretchBox * ) ), this, SLOT( mtxTry_userBoxMoved( StretchBox * ) ) );
    connect( sb, SIGNAL( rejectMouse( StretchBox * ) ), this, SLOT( userBoxRejectMouse( StretchBox * ) ) );
    connect( sb, SIGNAL( remove( StretchBox * ) ), this, SLOT( userBoxRemove( StretchBox * ) ) );
    connect( sb, SIGNAL( selected( StretchBox * ) ), this, SLOT( mtxTry_userBoxSelected( StretchBox * ) ) );
    connect( sb, SIGNAL( deSelected( StretchBox * ) ), this, SLOT( userBoxDeSelected( StretchBox * ) ) );

    m_qgs->addItem( sb );
}

void rtimvMainWindow::mtxTry_userBoxSize( StretchBox *sb )
{
    char tmp[256];
    snprintf( tmp, 256, "%0.1f x %0.1f", sb->rect().width(), sb->rect().height() );

    QRectF sbr = sb->sceneBoundingRect();
    QPoint qr = ui.graphicsView->mapFromScene( QPointF( sbr.x(), sbr.y() ) );

    ui.graphicsView->userItemSizeText( tmp, qr );

    mtxTry_fontLuminance( ui.graphicsView->userItemSize() );
}

void rtimvMainWindow::userBoxCross( StretchBox *sb )
{
    userItemCross( sb->pos(), sb->rect(), sb->pen() );
}

void rtimvMainWindow::mtxTry_userBoxMouseCoords( StretchBox *sb )
{
    QRectF sbr = sb->sceneBoundingRect();
    QPointF qr = QPointF( sbr.x() + 0.5 * sbr.width(), sbr.y() + 0.5 * sbr.height() );

    m_userItemMouseViewX = qr.x();
    m_userItemMouseViewY = qr.y();

    m_offsetItemMouseCoordsX = false;
    m_offsetItemMouseCoordsY = false;

    m_userItemXCen = sb->rect().x() + sb->pos().x() + sb->rect().width() * 0.5;
    m_userItemYCen = sb->rect().y() + sb->pos().y() + sb->rect().height() * 0.5;

    mtxTry_userItemMouseCoords( m_userItemMouseViewX, m_userItemMouseViewY, m_userItemXCen, m_userItemYCen );
}

void rtimvMainWindow::addStretchBox( StretchBox *sb )
{
    if( sb == nullptr )
        return;

    // m_userBoxes.insert(sb);

    connect( sb, SIGNAL( rejectMouse( StretchBox * ) ), this, SLOT( userBoxRejectMouse( StretchBox * ) ) );
    connect( sb, SIGNAL( remove( StretchBox * ) ), this, SLOT( userBoxRemove( StretchBox * ) ) );

    m_qgs->addItem( sb );
}

void rtimvMainWindow::mtxTry_userBoxMoved( StretchBox *sb )
{
    mtxTry_userBoxSize( sb );
    mtxTry_userBoxMouseCoords( sb );
    userBoxCross( sb );
}

void rtimvMainWindow::userBoxRejectMouse( StretchBox *sb )
{
    if( sb != m_colorBox && m_colorBox )
    {
        sb->stackBefore( m_colorBox );
    }

    if( sb != m_statsBox && m_statsBox )
    {
        sb->stackBefore( m_statsBox );
    }

    for( auto &ubit : m_userBoxes )
    {
        if( sb != ubit )
            sb->stackBefore( ubit );
    }

    for( auto &ucit : m_userCircles )
    {
        sb->stackBefore( ucit );
    }

    for( auto &ulit : m_userLines )
    {
        sb->stackBefore( ulit );
    }
}

void rtimvMainWindow::userBoxRemove( StretchBox *sb )
{
    if( sb == m_statsBox || sb == m_colorBox )
    {
        return;
    }

    m_userBoxes.erase( sb ); // Remove it from our list
    m_qgs->removeItem( sb ); // Remove it from the scene
    sb->deleteLater();       // clean it up after we're no longer in an async function

    ui.graphicsView->userItemSize()->setVisible( false );
    ui.graphicsView->userItemMouseCoords()->setVisible( false );
    m_objCenH->setVisible( false );
    m_objCenV->setVisible( false );
    m_nullMouseCoords = false;
    m_userItemSelected = false;
}

void rtimvMainWindow::mtxTry_userBoxSelected( StretchBox *sb )
{
    m_userItemXCen = sb->rect().x() + sb->pos().x() + 0.5 * sb->rect().width();
    m_userItemYCen = sb->rect().y() + sb->pos().y() + 0.5 * sb->rect().height();

    userItemSelected( ui.graphicsView->userItemDefColor(), true, true, true );
    mtxTry_userBoxMoved( sb );
}

void rtimvMainWindow::userBoxDeSelected( StretchBox *sb )
{
    static_cast<void>( sb );
    ui.graphicsView->userItemSize()->setVisible( false );
    ui.graphicsView->userItemMouseCoords()->setVisible( false );
    m_objCenH->setVisible( false );
    m_objCenV->setVisible( false );
    m_nullMouseCoords = false;
    m_userItemSelected = false;
}

/*---- User Circles ----*/

void rtimvMainWindow::addUserCircle()
{
    float w;

    float znx = m_nx / m_zoomLevel;
    float zny = m_ny / m_zoomLevel;
    if( znx < zny )
        w = znx / 4;
    else
        w = zny / 4;

    std::pair<std::unordered_set<StretchCircle *>::iterator, bool> it = m_userCircles.insert(
        new StretchCircle( ui.graphicsView->xCen() - w / 2, ui.graphicsView->yCen() - w / 2, w, w ) );

    StretchCircle *sc = *it.first;

    sc->setPenColor( "lime" );
    sc->setEdgeTol( m_userItemEdgeTol / m_screenZoom / m_zoomLevel );
    sc->setPenWidth( m_userItemLineWidth / m_screenZoom / m_zoomLevel );

    sc->setStretchable( true );
    sc->setVisible( true );

    connect( sc, SIGNAL( resized( StretchCircle * ) ), this, SLOT( userCircleMoved( StretchCircle * ) ) );
    connect( sc, SIGNAL( moved( StretchCircle * ) ), this, SLOT( userCircleMoved( StretchCircle * ) ) );
    connect( sc, SIGNAL( mouseIn( StretchCircle * ) ), this, SLOT( userCircleMoved( StretchCircle * ) ) );
    connect( sc, SIGNAL( rejectMouse( StretchCircle * ) ), this, SLOT( userCircleRejectMouse( StretchCircle * ) ) );
    connect( sc, SIGNAL( remove( StretchCircle * ) ), this, SLOT( userCircleRemove( StretchCircle * ) ) );
    connect( sc, SIGNAL( selected( StretchCircle * ) ), this, SLOT( userCircleSelected( StretchCircle * ) ) );
    connect( sc, SIGNAL( deSelected( StretchCircle * ) ), this, SLOT( userCircleDeSelected( StretchCircle * ) ) );

    m_qgs->addItem( sc );
}

void rtimvMainWindow::userCircleSize( StretchCircle *sc )
{
    char tmp[32];
    snprintf( tmp, sizeof( tmp ), "r=%0.1f", sc->radius() );

    // Take scene coordinates to viewport coordinates.
    QRectF sbr = sc->sceneBoundingRect();
    QPoint qr = ui.graphicsView->mapFromScene( QPointF( sbr.x() + sc->rect().width() * 0.5 - sc->radius() * 0.707,
                                                        sbr.y() + sc->rect().height() * 0.5 - sc->radius() * 0.707 ) );

    ui.graphicsView->userItemSizeText( tmp, qr );

    mtxTry_fontLuminance( ui.graphicsView->userItemSize() );
}

void rtimvMainWindow::userCircleCross( StretchCircle *sc )
{
    userItemCross( sc->pos(), sc->rect(), sc->pen() );
}

void rtimvMainWindow::mtxTry_userCircleMouseCoords( StretchCircle *sc )
{
    QRectF sbr = sc->sceneBoundingRect();
    QPointF qr = QPointF( sbr.x() + 0.5 * sbr.width(), sbr.y() + 0.5 * sbr.height() );

    m_offsetItemMouseCoordsX = false;
    m_offsetItemMouseCoordsY = false;

    m_userItemMouseViewX = qr.x();
    m_userItemMouseViewY = qr.y();

    mtxTry_userItemMouseCoords( m_userItemMouseViewX, m_userItemMouseViewY, m_userItemXCen, m_userItemYCen );
}

void rtimvMainWindow::addStretchCircle( StretchCircle *sc )
{
    if( sc == nullptr )
        return;

    m_userCircles.insert( sc );

    connect( sc, SIGNAL( rejectMouse( StretchCircle * ) ), this, SLOT( userCircleRejectMouse( StretchCircle * ) ) );
    connect( sc, SIGNAL( remove( StretchCircle * ) ), this, SLOT( userCircleRemove( StretchCircle * ) ) );

    m_qgs->addItem( sc );
}

void rtimvMainWindow::userCircleMoved( StretchCircle *sc )
{
    userCircleSize( sc );
    userCircleCross( sc );
    mtxTry_userCircleMouseCoords( sc );
}

void rtimvMainWindow::userCircleRejectMouse( StretchCircle *sc )
{
    if( m_colorBox )
    {
        sc->stackBefore( m_colorBox );
    }

    if( m_statsBox )
    {
        sc->stackBefore( m_statsBox );
    }

    for( auto &ubit : m_userBoxes )
    {
        sc->stackBefore( ubit );
    }

    for( auto &ucit : m_userCircles )
    {
        if( sc != ucit )
            sc->stackBefore( ucit );
    }

    for( auto &ulit : m_userLines )
    {
        sc->stackBefore( ulit );
    }
}

void rtimvMainWindow::userCircleRemove( StretchCircle *sc )
{
    m_userCircles.erase( sc ); // Remove it from our list
    m_qgs->removeItem( sc );   // Remove it from the scene
    sc->deleteLater();         // clean it up after we're no longer in an asynch function

    ui.graphicsView->userItemSize()->setVisible( false );
    ui.graphicsView->userItemMouseCoords()->setVisible( false );
    m_objCenH->setVisible( false );
    m_objCenV->setVisible( false );
    m_userItemSelected = false;
}

void rtimvMainWindow::userCircleSelected( StretchCircle *sc )
{
    m_userItemXCen = sc->rect().x() + sc->pos().x() + 0.5 * sc->rect().width();
    m_userItemYCen = sc->rect().y() + sc->pos().y() + 0.5 * sc->rect().height();

    userItemSelected( ui.graphicsView->userItemDefColor(), true, true, true );
    userCircleMoved( sc );
}

void rtimvMainWindow::userCircleDeSelected( StretchCircle *sb )
{
    static_cast<void>( sb );
    ui.graphicsView->userItemSize()->setVisible( false );
    ui.graphicsView->userItemMouseCoords()->setVisible( false );
    m_objCenH->setVisible( false );
    m_objCenV->setVisible( false );
    m_userItemSelected = false;
}

/*---- User Lines ----*/
void rtimvMainWindow::addUserLine()
{
    float w;

    float znx = m_nx / m_zoomLevel;
    float zny = m_ny / m_zoomLevel;
    if( znx < zny )
        w = znx / 4;
    else
        w = zny / 4;

    auto it = m_userLines.insert( new StretchLine( ui.graphicsView->xCen() - w / 2,
                                                   ui.graphicsView->yCen() - w / 2,
                                                   ui.graphicsView->xCen() + w / 2,
                                                   ui.graphicsView->yCen() + w / 2 ) );

    StretchLine *sl = *it.first;

    sl->setPenColor( "lime" );
    sl->setEdgeTol( m_userItemEdgeTol / m_screenZoom / m_zoomLevel );
    sl->setPenWidth( m_userItemLineWidth / m_screenZoom / m_zoomLevel );

    sl->setStretchable( true );
    sl->setVisible( true );

    connect( sl, SIGNAL( resized( StretchLine * ) ), this, SLOT( mtxTry_userLineMoved( StretchLine * ) ) );
    connect( sl, SIGNAL( moved( StretchLine * ) ), this, SLOT( mtxTry_userLineMoved( StretchLine * ) ) );
    connect( sl, SIGNAL( mouseIn( StretchLine * ) ), this, SLOT( mtxTry_userLineMoved( StretchLine * ) ) );
    connect( sl, SIGNAL( rejectMouse( StretchLine * ) ), this, SLOT( userLineRejectMouse( StretchLine * ) ) );
    connect( sl, SIGNAL( remove( StretchLine * ) ), this, SLOT( userLineRemove( StretchLine * ) ) );
    connect( sl, SIGNAL( selected( StretchLine * ) ), this, SLOT( mtxTry_userLineSelected( StretchLine * ) ) );
    connect( sl, SIGNAL( deSelected( StretchLine * ) ), this, SLOT( userLineDeSelected( StretchLine * ) ) );

    m_qgs->addItem( sl );
}

void rtimvMainWindow::mtxTry_userLineSize( StretchLine *sl )
{
    sharedLockT lock( m_calMutex );

    if( !m_qpmi )
        return;

    float ang = fmod( sl->angle() - 90 + northAngle(), 360.0 );

    if( ang < 0 )
        ang += 360.0;

    char tmp[256];
    snprintf( tmp, 256, "%0.1f\n%0.1f", sl->length(), ang );

    QPointF np = m_qpmi->mapFromItem( sl, sl->line().p2() );

    QFontMetrics fm( ui.graphicsView->userItemSize()->currentFont() );
    QSize fntsz = fm.size( 0, tmp );

    float offsetX = 0;
    float offsetY = 10;

    if( sl->angle() > 90 && sl->angle() < 270 )
    {
        offsetX = fntsz.width() + 20;
        offsetY = 0;
    }

    QRect rect(
        np.x() * m_screenZoom - offsetX, np.y() * m_screenZoom - offsetY, fntsz.width() + 5, fntsz.height() + 5 );
    ui.graphicsView->userItemSizeText( tmp, rect );

    mtxTry_fontLuminance( ui.graphicsView->userItemSize() );
}

void rtimvMainWindow::userLineHead( StretchLine *sl )
{
    m_userItemXCen = sl->line().x1();
    m_userItemYCen = sl->line().y1();

    m_lineHead->setPen( sl->pen() );

    float w = m_userLineHeadRad / m_screenZoom / m_zoomLevel;

    float lhx = sl->line().x1() - w;
    float lhy = sl->line().y1() - w;

    m_lineHead->setRect( lhx, lhy, 2 * w, 2 * w );
}

void rtimvMainWindow::mtxTry_userLineMouseCoords( StretchLine *sl )
{
    if( sl->angle() > 270 )
    {
        m_offsetItemMouseCoordsX = true;
        m_offsetItemMouseCoordsY = true;
    }
    else
    {
        m_offsetItemMouseCoordsX = false;
        m_offsetItemMouseCoordsY = false;
    }

    QPointF qr = QPointF( sl->line().x1(), sl->line().y1() );

    m_userItemMouseViewX = qr.x();
    m_userItemMouseViewY = qr.y();

    mtxTry_userItemMouseCoords( m_userItemMouseViewX, m_userItemMouseViewY, m_userItemXCen, m_userItemYCen );
}

void rtimvMainWindow::addStretchLine( StretchLine *sl )
{
    if( sl == nullptr )
        return;

    m_userLines.insert( sl );

    connect( sl, SIGNAL( rejectMouse( StretchLine * ) ), this, SLOT( userLineRejectMouse( StretchLine * ) ) );
    connect( sl, SIGNAL( remove( StretchLine * ) ), this, SLOT( userLineRemove( StretchLine * ) ) );

    m_qgs->addItem( sl );
}

void rtimvMainWindow::mtxTry_userLineMoved( StretchLine *sl )
{
    mtxTry_userLineSize( sl );
    userLineHead( sl );
    mtxTry_userLineMouseCoords( sl );
}

void rtimvMainWindow::userLineRejectMouse( StretchLine *sl )
{
    if( m_colorBox )
    {
        sl->stackBefore( m_colorBox );
    }

    if( m_statsBox )
    {
        sl->stackBefore( m_statsBox );
    }

    for( auto &ubit : m_userBoxes )
    {
        sl->stackBefore( ubit );
    }

    for( auto &ucit : m_userCircles )
    {
        sl->stackBefore( ucit );
    }

    for( auto &ulit : m_userLines )
    {
        if( sl != ulit )
            sl->stackBefore( ulit );
    }
}

void rtimvMainWindow::userLineRemove( StretchLine *sl )
{
    m_userLines.erase( sl ); // Remove it from our list
    m_qgs->removeItem( sl ); // Remove it from the scene
    sl->deleteLater();       // clean it up after we're no longer in an asynch function
    ui.graphicsView->userItemMouseCoords()->setVisible( false );
    ui.graphicsView->userItemSize()->setVisible( false );
    m_lineHead->setVisible( false );
    m_userItemSelected = false;
}

void rtimvMainWindow::mtxTry_userLineSelected( StretchLine *sl )
{
    m_userItemXCen = sl->line().x1();
    m_userItemYCen = sl->line().y1();

    m_lineHead->setVisible( true );
    userItemSelected( ui.graphicsView->userItemDefColor(), true, true, false );
    mtxTry_userLineMoved( sl );
}

void rtimvMainWindow::userLineDeSelected( StretchLine *sl )
{
    static_cast<void>( sl );
    ui.graphicsView->userItemSize()->setVisible( false );
    ui.graphicsView->userItemMouseCoords()->setVisible( false );
    m_lineHead->setVisible( false );
    m_userItemSelected = false;
}

float rtimvMainWindow::targetXc()
{
    return m_targetXc;
}

float rtimvMainWindow::targetYc()
{
    return m_targetYc;
}

bool rtimvMainWindow::targetVisible()
{
    return m_targetVisible;
}

void rtimvMainWindow::targetXc( float txc )
{
    if( txc < 0 )
        txc = 0;
    if( txc > 1 )
        txc = 1;

    m_targetXc = txc;

    setTarget();

    emit targetXcChanged( m_targetXc );
}

void rtimvMainWindow::targetYc( float tyc )
{
    if( tyc < 0 )
        tyc = 0;
    if( tyc > 1 )
        tyc = 1;

    m_targetYc = tyc;

    setTarget();

    emit targetYcChanged( m_targetYc );
}

void rtimvMainWindow::targetVisible( bool tv )
{
    if( m_targetVisible != tv )
    {
        if( tv )
        {
            ui.graphicsView->zoomText( "target on" );
            mtxTry_fontLuminance( ui.graphicsView->zoomText() );
        }
        else
        {
            ui.graphicsView->zoomText( "target off" );
            mtxTry_fontLuminance( ui.graphicsView->zoomText() );
        }
    }
    m_targetVisible = tv;
    setTarget();
    emit targetVisibleChanged( m_targetVisible );
}

void rtimvMainWindow::setTarget()
{
    if( !m_cenLineVert )
    {
        m_cenLineVert = m_qgs->addLine( QLineF( m_targetXc * nx(), 0, m_targetXc * nx(), ny() ), QColor( "lime" ) );
        m_cenLineHorz = m_qgs->addLine( QLineF( 0, ( 1.0 - m_targetYc ) * ny(), nx(), ( 1.0 - m_targetYc ) * ny() ),
                                        QColor( "lime" ) );
        if( m_targetVisible )
        {
            m_cenLineVert->setVisible( true );
            m_cenLineHorz->setVisible( true );
        }
        else
        {
            m_cenLineVert->setVisible( false );
            m_cenLineHorz->setVisible( false );
        }
    }
    else
    {
        m_cenLineVert->setLine( QLineF( m_targetXc * nx(), 0, m_targetXc * nx(), ny() ) );
        m_cenLineHorz->setLine( QLineF( 0, ( 1.0 - m_targetYc ) * ny(), nx(), ( 1.0 - m_targetYc ) * ny() ) );
        if( m_targetVisible )
        {
            m_cenLineVert->setVisible( true );
            m_cenLineHorz->setVisible( true );
        }
        else
        {
            m_cenLineVert->setVisible( false );
            m_cenLineHorz->setVisible( false );
        }
    }
}

void rtimvMainWindow::savingState( rtimv::savingState ss )
{
    if( ss == rtimv::savingState::on )
    {
        ui.graphicsView->saveBoxFontColor( "lightgreen" );
        ui.graphicsView->saveBox()->setText( "S" );
    }
    else if( ss == rtimv::savingState::waiting )
    {
        ui.graphicsView->saveBoxFontColor( "yellow" );
        ui.graphicsView->saveBox()->setText( "S" );
    }
    else
    {
        ui.graphicsView->saveBoxFontColor( "red" );
        ui.graphicsView->saveBox()->setText( "X" );
    }
}

void rtimvMainWindow::mtxL_postSetColorBoxActive( bool usba, const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    if( m_colorBox )
    {
        m_colorBox->setVisible( usba );
    }
}

void rtimvMainWindow::keyPressEvent( QKeyEvent *ke )
{
    // First deal with the control sequences
    if( ke->modifiers() & Qt::ControlModifier )
    {
        switch( ke->key() )
        {
        case Qt::Key_C:
            mtxUL_center();
            break;
        case Qt::Key_Plus:
            zoomLevel( zoomLevel() + 0.1 );
            break;
        case Qt::Key_Minus:
            zoomLevel( zoomLevel() - 0.1 );
            break;
        }
    }
    else // Finally deal with unmodified keys
    {
        char key = ke->text()[0].toLatin1();

        switch( ke->key() )
        {
        case Qt::Key_A:
            if( key == 'a' )
                return toggleAutoScale();
            break;
        case Qt::Key_B:
            if( key == 'b' )
                return addUserBox();
            break;
        case Qt::Key_C:
            if( key == 'c' )
                return addUserCircle();
            if( key == 'C' )
                return toggleCubeCtrl();
            break;
        case Qt::Key_D:
            if( key == 'D' )
                return toggleDarkSub();
            break;
        case Qt::Key_F:
            if( key == 'f' )
                return toggleFPSGage();
            break;
        case Qt::Key_H:
            if( key == 'h' )
                return toggleHelp();
            break;
        case Qt::Key_I:
            if( key == 'i' )
                return toggleInfo();
            break;
        case Qt::Key_L:
            if( key == 'l' )
                return addUserLine();
            if( key == 'L' )
                return toggleLogLinear();
            break;
        case Qt::Key_M:
            if( key == 'M' )
                return toggleApplyMask();
            break;
        case Qt::Key_N:
            if( key == 'n' )
                return toggleNorthArrow();
            break;
        case Qt::Key_P:
            if( key == 'p' )
                return launchControlPanel();
            break;
        case Qt::Key_R:
            if( key == 'r' )
                return reStretch();
            break;
        case Qt::Key_S:
            if( key == 's' )
                return toggleStatsBox();
            if( key == 'S' )
                return toggleApplySatMask();
            break;
        case Qt::Key_T:
            if( key == 't' )
                return toggleTarget();
            break;
        case Qt::Key_X:
            if( key == 'x' )
                return freezeRealTime();
            break;
        case Qt::Key_Z:
            if( key == 'z' )
                return toggleColorBox();
            break;
        case Qt::Key_1:
            return zoomLevel( 1.0 );
            break;
        case Qt::Key_2:
            return zoomLevel( 2.0 );
            break;
        case Qt::Key_3:
            return zoomLevel( 3.0 );
            break;
        case Qt::Key_4:
            return zoomLevel( 4.0 );
            break;
        case Qt::Key_5:
            return zoomLevel( 5.0 );
            break;
        case Qt::Key_6:
            return zoomLevel( 6.0 );
            break;
        case Qt::Key_7:
            return zoomLevel( 7.0 );
            break;
        case Qt::Key_8:
            return zoomLevel( 8.0 );
            break;
        case Qt::Key_9:
            return zoomLevel( 9.0 );
            break;
        case Qt::Key_BracketLeft:
            return squareDown();
            break;
        case Qt::Key_BracketRight:
            return squareUp();
        /*case Qt::Key_Up:
           return;*/
        case Qt::Key_W:
            if( key == 'w' )
            {
                if( m_borderWarningLevel == rtimv::warningLevel::normal )
                {
                    return borderWarningLevel( rtimv::warningLevel::info );
                }
                else if( m_borderWarningLevel == rtimv::warningLevel::info )
                {
                    return borderWarningLevel( rtimv::warningLevel::caution );
                }
                else if( m_borderWarningLevel == rtimv::warningLevel::caution )
                {
                    return borderWarningLevel( rtimv::warningLevel::warning );
                }
                if( m_borderWarningLevel == rtimv::warningLevel::warning )
                {
                    return borderWarningLevel( rtimv::warningLevel::alert );
                }
                else
                {
                    return borderWarningLevel( rtimv::warningLevel::normal );
                }
            }
            break;
        default:
            break;
        }
    }

    for( size_t n = 0; n < m_overlays.size(); ++n )
    {
        m_overlays[n]->keyPressEvent( ke );
    }
}

void rtimvMainWindow::autoScale( bool as )
{
    m_autoScale = as;
    emit autoScaleUpdated( m_autoScale );
    if( m_autoScale )
    {
        ui.graphicsView->zoomText( "autoscale on" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
    }
    else
    {
        ui.graphicsView->zoomText( "autoscale off" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
    }
}

void rtimvMainWindow::toggleAutoScale()
{
    if( m_autoScale )
    {
        autoScale( false );
    }
    else
    {
        autoScale( true );
    }
}

void rtimvMainWindow::mtxUL_center()
{
    sharedLockT lock( m_calMutex );

    mtxL_setViewCen( .5, .5, lock );

    post_zoomLevel();

    ui.graphicsView->zoomText( "centered" );

    mtxL_fontLuminance( ui.graphicsView->zoomText(), lock );
}

void rtimvMainWindow::toggleColorBox()
{
    if( !colorBoxActive || !m_colorBox )
    {
        toggleColorBoxOn();
    }
    else
    {
        toggleColorBoxOff();
    }
}

void rtimvMainWindow::toggleColorBoxOn()
{
    if( m_colorBox == nullptr )
    {
        float w;
        if( m_nx < m_ny )
            w = ( m_nx / m_zoomLevel ) / 4;
        else
            w = ( m_ny / m_zoomLevel ) / 4;

        colorBox_i0( 0.5 * (m_nx)-w / 2 );
        colorBox_i1( colorBox_i0() + w );
        colorBox_j0( 0.5 * (m_ny)-w / 2 );
        colorBox_j1( colorBox_j0() + w );

        m_colorBox = new StretchBox( colorBox_i0(), colorBox_j0(), w, w );

        m_colorBox->setPenColor( RTIMV_DEF_COLORBOXCOLOR );
        m_colorBox->setPenWidth( m_userItemLineWidth / m_screenZoom / m_zoomLevel );
        m_colorBox->setEdgeTol( m_userItemEdgeTol / m_screenZoom / m_zoomLevel );

        m_colorBox->setVisible( false );
        m_colorBox->setStretchable( true );
        m_colorBox->setRemovable( true );

        connect( m_colorBox, SIGNAL( resized( StretchBox * ) ), this, SLOT( mtxTry_colorBoxMoved( StretchBox * ) ) );
        connect( m_colorBox, SIGNAL( moved( StretchBox * ) ), this, SLOT( mtxTry_colorBoxMoved( StretchBox * ) ) );
        connect( m_colorBox, SIGNAL( rejectMouse( StretchBox * ) ), this, SLOT( userBoxRejectMouse( StretchBox * ) ) );
        connect(
            m_colorBox, SIGNAL( selected( StretchBox * ) ), this, SLOT( mtxTry_colorBoxSelected( StretchBox * ) ) );
        connect( m_colorBox, SIGNAL( deSelected( StretchBox * ) ), this, SLOT( colorBoxDeselected( StretchBox * ) ) );
        connect( m_colorBox, SIGNAL( remove( StretchBox * ) ), this, SLOT( colorBoxRemove( StretchBox * ) ) );

        m_qgs->addItem( m_colorBox );
    }

    m_colorBox->setVisible( true );

    sharedLockT lock( m_calMutex );
    mtxL_setColorBoxActive( true, lock );
    lock.unlock();

    ui.graphicsView->zoomText( "color box scale" );
    mtxTry_fontLuminance( ui.graphicsView->zoomText() );
}

void rtimvMainWindow::toggleColorBoxOff()
{
    if( m_colorBox->isSelected() )
    {
        colorBoxDeselected( m_colorBox );
    }

    m_colorBox->setVisible( false );

    sharedLockT lock( m_calMutex );
    mtxL_setColorBoxActive( false, lock );
    lock.unlock();

    ui.graphicsView->zoomText( "global scale" );
    mtxTry_fontLuminance( ui.graphicsView->zoomText() );
}

void rtimvMainWindow::toggleStatsBox()
{
    if( m_statsBox == nullptr )
    {
        float w;
        if( m_nx < m_ny )
            w = m_nx / 4;
        else
            w = m_ny / 4;

        m_statsBox = new StretchBox( 0.5 * (m_nx)-w / 2, 0.5 * (m_ny)-w / 2, w, w );
        m_statsBox->setPenColor( RTIMV_DEF_STATSBOXCOLOR );
        m_statsBox->setPenWidth( m_userItemLineWidth / m_screenZoom / m_zoomLevel );
        m_statsBox->setEdgeTol( m_userItemEdgeTol / m_screenZoom / m_zoomLevel );

        m_statsBox->setVisible( false );
        m_statsBox->setStretchable( true );
        m_statsBox->setRemovable( true );
        // m_userBoxes.insert(m_statsBox);
        connect( m_statsBox, SIGNAL( resized( StretchBox * ) ), this, SLOT( mtxTry_statsBoxMoved( StretchBox * ) ) );
        connect( m_statsBox, SIGNAL( moved( StretchBox * ) ), this, SLOT( mtxTry_statsBoxMoved( StretchBox * ) ) );
        connect( m_statsBox, SIGNAL( rejectMouse( StretchBox * ) ), this, SLOT( userBoxRejectMouse( StretchBox * ) ) );
        connect(
            m_statsBox, SIGNAL( selected( StretchBox * ) ), this, SLOT( mtxTry_statsBoxSelected( StretchBox * ) ) );
        connect( m_statsBox, SIGNAL( deSelected( StretchBox * ) ), this, SLOT( userBoxDeSelected( StretchBox * ) ) );
        connect( m_statsBox, SIGNAL( remove( StretchBox * ) ), this, SLOT( statsBoxRemove( StretchBox * ) ) );
        m_qgs->addItem( m_statsBox );
    }

    if( m_statsBox->isVisible() )
    {
        doHideStatsBox();
        ui.graphicsView->zoomText( "stats off" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
        if( imcp )
        {
            imcp->statsBoxButtonState = false;
            imcp->ui.statsBoxButton->setText( "Show Stats Box" );
        }
    }
    else
    {
        doLaunchStatsBox();
        ui.graphicsView->zoomText( "stats on" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
        if( imcp )
        {
            imcp->statsBoxButtonState = true;
            imcp->ui.statsBoxButton->setText( "Hide Stats Box" );
        }
    }
}

void rtimvMainWindow::toggleNorthArrow()
{
    if( !m_northArrow )
        return;

    if( !m_northArrowEnabled )
    {
        m_northArrow->setVisible( false );
        m_northArrowTip->setVisible( false );
        return;
    }

    if( m_northArrow->isVisible() )
    {
        m_northArrow->setVisible( false );
        m_northArrowTip->setVisible( false );
        ui.graphicsView->zoomText( "North Off" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
    }
    else
    {
        m_northArrow->setVisible( true );
        m_northArrowTip->setVisible( true );
        ui.graphicsView->zoomText( "North On" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
    }
}

void rtimvMainWindow::showFPSGage( bool sfg )
{
    m_showFPSGage = sfg;
    if( m_showFPSGage )
    {
        ui.graphicsView->zoomText( "fps gage on" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
    }
    else
    {
        ui.graphicsView->fpsGageText( "" );
        ui.graphicsView->zoomText( "fps gage off" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
    }
}

void rtimvMainWindow::toggleFPSGage()
{
    if( m_showFPSGage )
    {
        return showFPSGage( false );
    }
    else
    {
        return showFPSGage( true );
    }
}

void rtimvMainWindow::setDarkSub( bool ds )
{
    m_subtractDark = ds;
    if( m_subtractDark )
    {
        ui.graphicsView->zoomText( "dark sub. on" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
    }
    else
    {
        ui.graphicsView->zoomText( "dark sub. off" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
    }

    mtxUL_changeImdata( true ); //have to trigger refresh of cal data
}

void rtimvMainWindow::toggleDarkSub()
{
    if( m_subtractDark )
    {
        return setDarkSub( false );
    }
    else
    {
        return setDarkSub( true );
    }

}

void rtimvMainWindow::setApplyMask( bool am )
{
    m_applyMask = am;
    if( m_applyMask )
    {
        ui.graphicsView->zoomText( "mask on" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
    }
    else
    {
        ui.graphicsView->zoomText( "mask off" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
    }

    mtxUL_changeImdata( true ); //have to trigger refresh of cal data
}

void rtimvMainWindow::toggleApplyMask()
{
    if( m_applyMask )
    {
        return setApplyMask( false );
    }
    else
    {
        return setApplyMask( true );
    }

}

void rtimvMainWindow::setApplySatMask( bool as )
{
    m_applySatMask = as;
    if( m_applySatMask )
    {
        ui.graphicsView->zoomText( "sat mask on" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
    }
    else
    {
        ui.graphicsView->zoomText( "sat mask off" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
    }

    mtxUL_changeImdata( true ); //have to trigger refresh of cal data
}

void rtimvMainWindow::toggleApplySatMask()
{
    if( m_applySatMask )
    {
        return setApplySatMask( false );
    }
    else
    {
        return setApplySatMask( true );
    }
}

void rtimvMainWindow::toggleLogLinear()
{
    int s = get_cbStretch();

    if( s == stretchLog )
    {
        set_cbStretch( stretchLinear );
        ui.graphicsView->zoomText( "linear stretch" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
        reStretch();
    }
    else
    {
        set_cbStretch( stretchLog );
        ui.graphicsView->zoomText( "log stretch" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
        reStretch();
    }
}

void rtimvMainWindow::toggleTarget()
{
    if( m_targetVisible )
    {
        targetVisible( false );
        ui.graphicsView->zoomText( "target off" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
    }
    else
    {
        targetVisible( true );
        ui.graphicsView->zoomText( "target on" );
        mtxTry_fontLuminance( ui.graphicsView->zoomText() );
    }
}

std::string rtimvMainWindow::generateHelp()
{
    std::string help;
    help = "                       rtimv online help                    \n";
    help += "                     press 'h' to exit help        \n";
    help += "\n";
    help += "Shortcuts:\n";
    //      "01234567890123456789012345678901234567890123456789012345678901234567890123456789
    help += "a: toggle autoscale           b: add box                     \n";
    help += "c: add circle                 f: toggle FPS gauge            \n";
    help += "h: toggle help                i: toggle info                 \n";
    help += "l: add line                                                  \n";
    help += "n: toggle north arrow         p: launch control panel        \n";
    help += "r: re-stretch color table     s: toggle statistics box       \n";
    help += "t: toggle target cross        x: freeze real-time            \n";
    help += "z: toggle color box\n";

    help += "\n";
    help += "C: toggle cube control        D: toggle dark subtraction     \n";
    help += "L: toggle log scale           M: toggle mask                 \n";
    help += "S: toggle saturation mask      \n";

    help += "\n";
    help += "1-9: change zoom level        ctrl +: zoom in\n";
    help += "                              ctrl -: zoom out\n";
    help += "\n";
    help += "[: fit horizontal             ]: fit vertical\n";
    help += "\n";
    help += "ctrl c: center image          delete: remove selected object \n";

    return help;
}

void rtimvMainWindow::toggleHelp()
{
    if( m_helpVisible )
    {
        ui.graphicsView->helpText()->setVisible( false );
        m_helpVisible = false;
    }
    else
    {
        std::string help = generateHelp();
        ui.graphicsView->helpTextText( help.c_str() );
        ui.graphicsView->helpText()->setVisible( true );
        m_helpVisible = true;
        m_infoVisible = false;
    }
}

std::string rtimvMainWindow::generateInfo()
{
    std::string info;
    info = "                       rtimv online info                   \n";
    info += "                     press 'i' to exit info        \n";
    info += "\n";
    info += "Images:\n";
    //      "01234567890123456789012345678901234567890123456789012345678901234567890123456789
    if( m_images[0] != nullptr )
    {
        std::vector<std::string> iinfo = m_images[0]->info();
        if( iinfo.size() > 0 )
        {
            info += "  image:   " + iinfo[0] + "\n";
        }

        for( size_t i = 1; i < iinfo.size(); ++i )
        {
            info += "           " + iinfo[i] + "\n";
        }
    }
    else
    {
        info += "  image:   \n";
    }
    if( m_images[1] != nullptr )
    {
        std::vector<std::string> iinfo = m_images[1]->info();
        if( iinfo.size() > 0 )
        {
            info += "  dark:    " + iinfo[0] + "\n";
        }

        for( size_t i = 1; i < iinfo.size(); ++i )
        {
            info += "           " + iinfo[i] + "\n";
        }
    }
    else
    {
        info += "  dark:    \n";
    }
    if( m_images[2] != nullptr )
    {
        std::vector<std::string> iinfo = m_images[2]->info();
        if( iinfo.size() > 0 )
        {
            info += "  mask:    " + iinfo[0] + "\n";
        }

        for( size_t i = 1; i < iinfo.size(); ++i )
        {
            info += "           " + iinfo[i] + "\n";
        }
    }
    else
    {
        info += "  mask:    \n";
    }
    if( m_images[3] != nullptr )
    {
        std::vector<std::string> iinfo = m_images[3]->info();
        if( iinfo.size() > 0 )
        {
            info += "  sat-mask: " + iinfo[0] + "\n";
        }

        for( size_t i = 1; i < iinfo.size(); ++i )
        {
            info += "            " + iinfo[i] + "\n";
        }
    }
    else
    {
        info += "  satMask: \n";
    }

    info += "\n";
    info += "Plugins:\n";
    for( size_t p = 0; p < m_plugins.size(); ++p )
    {
        if( m_plugins[p] != nullptr )
        {
            std::vector<std::string> pinfo = m_plugins[p]->info();
            for( size_t i = 0; i < pinfo.size(); ++i )
            {
                info += "  " + pinfo[i] + "\n";
            }
        }
    }
    info += "\n";

    return info;
}

void rtimvMainWindow::toggleInfo()
{
    if( m_infoVisible )
    {
        ui.graphicsView->helpText()->setVisible( false );
        m_infoVisible = false;
    }
    else
    {
        std::string info = generateInfo();
        ui.graphicsView->helpTextText( info.c_str() );
        ui.graphicsView->helpText()->setVisible( true );
        m_infoVisible = true;
        m_helpVisible = false;
    }
}

void rtimvMainWindow::borderWarningLevel( rtimv::warningLevel lvl )
{
    m_borderWarningLevel = lvl;

    if( m_borderBox == nullptr )
    {
        m_borderBox = new StretchBox();

        m_borderBox->setVisible( false );
        m_borderBox->setStretchable( false );
        m_borderBox->setRemovable( false );
        m_qgs->addItem( m_borderBox );
    }

    if( lvl == rtimv::warningLevel::alert )
    {
        m_borderBox->setPenColor( "magenta" );
        m_borderBox->setVisible( true );
    }
    else if( lvl == rtimv::warningLevel::warning )
    {
        m_borderBox->setPenColor( "red" );
        m_borderBox->setVisible( true );
    }
    else if( lvl == rtimv::warningLevel::caution )
    {
        m_borderBox->setPenColor( "yellow" );
        m_borderBox->setVisible( true );
    }
    else if( lvl == rtimv::warningLevel::info )
    {
        m_borderBox->setPenColor( "white" );
        m_borderBox->setVisible( true );
    }
    else //(lvl == rtimv::warningLevel::normal)
    {
        m_borderBox->setVisible( false );
    }

    setBorderBox();
}

void rtimvMainWindow::setBorderBox()
{
    if( !m_borderBox )
        return;
    if( !m_borderBox->isVisible() )
        return;

    float w, h;
    w = m_nx / m_zoomLevel;
    h = m_ny / m_zoomLevel;

    // Change pen so it looks right relative to size of pixels
    float pw = ( m_warningBorderWidth / m_screenZoom ) / m_zoomLevel;

    m_borderBox->setPenWidth( pw );

    // The must offset slightly to make sure it is displayed
    m_borderBox->setRect( ui.graphicsView->xCen() - w / 2 + 0.25 * pw,
                          ui.graphicsView->yCen() - w / 2 + 0.25 * pw,
                          w - 0.5 * pw,
                          h - 0.5 * pw );
}

template <typename realT>
realT sRGBtoLinRGB( int rgb )
{
    realT V = ( (realT)rgb ) / 255.0;

    if( V <= 0.0405 )
        return V / 12.92;

    return pow( ( V + 0.055 ) / 1.055, 2.4 );
}

template <typename realT>
realT linRGBtoLuminance( realT linR, realT linG, realT linB )
{
    return 0.2126 * linR + 0.7152 * linG + 0.0722 * linB;
}

template <typename realT>
realT pLightness( realT lum )
{
    if( lum <= static_cast<realT>( 216 ) / static_cast<realT>( 24389 ) )
    {
        return lum * static_cast<realT>( 24389 ) / static_cast<realT>( 27 );
    }

    return pow( lum, static_cast<realT>( 1 ) / static_cast<realT>( 3 ) ) * 116 - 16;
}

void rtimvMainWindow::mtxL_fontLuminance( QTextEdit *qte, const sharedLockT &lock, bool print )
{
    assert( lock.owns_lock() );

    QPointF ptul = ui.graphicsView->mapToScene( qte->x(), qte->y() );
    QPointF ptlr = ui.graphicsView->mapToScene( qte->x() + qte->width(), qte->y() + qte->height() );

    unsigned myul = ptul.y();
    if( myul > m_ny - 1 )
        myul = 0;

    unsigned mxul = ptul.x();
    if( mxul > m_nx - 1 )
        mxul = 0;

    unsigned mylr = ptlr.y();
    if( mylr > m_ny - 1 )
        mylr = m_ny - 1;

    unsigned mxlr = ptlr.x();
    if( mxlr > m_nx - 1 )
        mxlr = m_nx - 1;

    if( mxul == 0 && myul == 0 && mxlr == 0 && mylr == 0 )
        return;
    if( mxul == mxlr || myul == mylr )
        return;

    double avgLum = 0;
    int N = 0;
    for( unsigned x = mxul; x <= mxlr; ++x )
    {
        for( unsigned y = myul; y <= mylr; ++y )
        {
            avgLum += pow( m_lightness[m_qim->pixelIndex( x, y )], m_lumPwr );
            ++N;
        }
    }
    avgLum /= N;
    avgLum = pow( avgLum, 1.0 / m_lumPwr );

    if( print )
        std::cerr << "avgLum: " << avgLum << "\n";

    if( avgLum <= m_lumThresh )
    {
        qte->setTextBackgroundColor( QColor( 0, 0, 0, 0 ) );
    }
    else if( avgLum < m_lumMax )
    {
        int op = ( avgLum - m_lumThresh ) / ( m_lumMax - m_lumThresh ) * m_opacityMax + 0.5;
        qte->setTextBackgroundColor( QColor( 0, 0, 0, op ) );
    }
    else
    {
        qte->setTextBackgroundColor( QColor( 0, 0, 0, m_opacityMax ) );
    }

    return;
}

void rtimvMainWindow::mtxTry_fontLuminance( QTextEdit *qte, bool print )
{
    sharedLockT lock( m_calMutex );

    return mtxL_fontLuminance( qte, lock, print );
}

void rtimvMainWindow::mtxTry_fontLuminance()
{
    sharedLockT lock( m_calMutex );

    mtxL_fontLuminance( ui.graphicsView->fpsGage(), lock );

    mtxL_fontLuminance( ui.graphicsView->zoomText(), lock );

    if( !m_nullMouseCoords )
    {
        if( m_showStaticCoords )
        {
            mtxL_fontLuminance( ui.graphicsView->textCoordX(), lock );
            mtxL_fontLuminance( ui.graphicsView->textCoordY(), lock );
            mtxL_fontLuminance( ui.graphicsView->textPixelVal(), lock );
        }

        if( m_showToolTipCoords )
        {
            mtxL_fontLuminance( ui.graphicsView->mouseCoords(), lock );
        }
    }

    for( size_t n = 0; n < ui.graphicsView->statusTextNo(); ++n )
    {
        if( ui.graphicsView->statusText( n )->toPlainText().size() > 0 )
        {
            mtxL_fontLuminance( ui.graphicsView->statusText( n ), lock );
        }
    }

    mtxL_fontLuminance( ui.graphicsView->saveBox(), lock );

    return;
}

int rtimvMainWindow::loadPlugin( QObject *plugin )
{
    auto rdi = qobject_cast<rtimvDictionaryInterface *>( plugin );
    rtimvInterface *ri{ nullptr };

    if( rdi )
    {
        int arv = rdi->attachDictionary( &m_dictionary, config );

        if( arv < 0 )
        {
            std::cerr << "Error from attachDictionary: " << arv << "\n";
            return arv;
        }
        else if( arv > 0 )
        {
            return arv;
        }
        else
        {
            ri = rdi;
        }
    }

    auto roi = qobject_cast<rtimvOverlayInterface *>( plugin );
    if( roi )
    {
        rtimvOverlayAccess roa;
        roa.m_mainWindowObject = this;
        roa.m_colorBox = m_colorBox;
        roa.m_statsBox = m_statsBox;
        roa.m_userBoxes = &m_userBoxes;
        roa.m_userCircles = &m_userCircles;
        roa.m_userLines = &m_userLines;
        roa.m_graphicsView = ui.graphicsView;
        roa.m_dictionary = &m_dictionary;

        int arv = roi->attachOverlay( roa, config );

        if( arv < 0 )
        {
            std::cerr << "Error from attachOverlay: " << arv << "\n";
            return arv;
        }
        else if( arv > 0 )
        {
            return arv;
        }
        else
        {
            m_overlays.push_back( roi );
            ri = roi;
        }
    }

    // If ri is not null, we store it.  Note this could be many types of interfaces at once.
    if( ri )
    {
        m_plugins.push_back( ri );
        return 0;
    }
    else
    {
        return 2; // this means no valid plugin matches
    }
}

bool rtimvMainWindow::eventFilter( QObject *obj, QEvent *event )
{
    // Events from the QGraphiscScene
    if( obj == m_qgs )
    {
        if( event->type() == QEvent::KeyPress )
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>( event );
            if( keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Right || keyEvent->key() == Qt::Key_Up ||
                keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_PageUp ||
                keyEvent->key() == Qt::Key_PageDown )
            {
                // We still want to take these for use in the main window, but don't want QGS to process them.
                keyPressEvent( keyEvent );
                return true;
            }
        }
    }
    return false;
}
