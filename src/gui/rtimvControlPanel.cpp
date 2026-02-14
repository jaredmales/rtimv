#include "rtimvControlPanel.hpp"

rtimvControlPanel::rtimvControlPanel()
{
}

rtimvControlPanel::rtimvControlPanel( rtimvMainWindow *v, Qt::WindowFlags f ) : QWidget( 0, f )
{
    m_ui.setupUi( this );
    m_imv = v;

    setupMode();
    setupCombos();
    m_ui.tabWidget->setCurrentIndex( 0 );

    m_qgsView = new QGraphicsScene();
    m_qgsEmpty = new QGraphicsScene();

    m_ui.pointerView->setScene( m_qgsEmpty );
    m_pointerViewEmpty = true;

    m_qpmiView = m_qgsView->addPixmap( *( v->getPixmap() ) );

    m_ui.viewView->setScene( m_qgsView );

    m_viewLineVert =
        m_qgsView->addLine( QLineF( .5 * m_imv->nx(), 0, .5 * m_imv->nx(), m_imv->ny() ), QColor( "lime" ) );
    m_viewLineHorz =
        m_qgsView->addLine( QLineF( 0, .5 * m_imv->ny(), m_imv->nx(), .5 * m_imv->ny() ), QColor( "lime" ) );
    m_viewBox = new StretchBox( 0, 0, m_imv->nx(), m_imv->ny() );
    m_viewBox->setPen( QPen( "lime" ) );
    m_viewBox->setFlag( QGraphicsItem::ItemIsSelectable, true );
    m_viewBox->setFlag( QGraphicsItem::ItemIsMovable, true );
    m_viewBox->setStretchable( false );
    m_viewBox->setEdgeTol( m_imv->nx() );
    m_qgsView->addItem( m_viewBox );

    double viewZoom = (double)m_ui.viewView->width() / (double)m_imv->nx();
    m_ui.viewView->scale( viewZoom, viewZoom );

    m_pointerViewFixed = false;
    m_pointerViewWaiting = false;

    connect( m_imv, SIGNAL( showToolTipCoordsChanged( bool ) ), this, SLOT( showToolTipCoordsChanged( bool ) ) );
    connect( this, SIGNAL( showToolTipCoords( bool ) ), m_imv, SLOT( showToolTipCoords( bool ) ) );
    connect( m_imv, SIGNAL( showStaticCoordsChanged( bool ) ), this, SLOT( showStaticCoordsChanged( bool ) ) );
    connect( this, SIGNAL( showStaticCoords( bool ) ), m_imv, SLOT( showStaticCoords( bool ) ) );

    connect( m_imv, SIGNAL( targetXcChanged( float ) ), this, SLOT( targetXcChanged( float ) ) );
    connect( m_imv, SIGNAL( targetYcChanged( float ) ), this, SLOT( targetYcChanged( float ) ) );
    connect( m_imv, SIGNAL( targetVisibleChanged( bool ) ), this, SLOT( targetVisibleChanged( bool ) ) );

    connect( this, SIGNAL( targetXc( float ) ), m_imv, SLOT( targetXc( float ) ) );
    connect( this, SIGNAL( targetYc( float ) ), m_imv, SLOT( targetYc( float ) ) );
    connect( this, SIGNAL( targetVisible( bool ) ), m_imv, SLOT( targetVisible( bool ) ) );

    init_panel();

    m_statsBoxButtonState = false;
}

void rtimvControlPanel::setupMode()
{
    m_viewViewMode = ViewViewNoImage;       // ViewViewEnabled;
    m_pointerViewMode = PointerViewOnPress; // PointerViewEnabled;
}

void rtimvControlPanel::setupCombos()
{
    m_ui.scaleModeCombo->insertItem( static_cast<int>( rtimv::colormode::minmaxglobal ), "Min/Max Global" );
    m_ui.scaleModeCombo->insertItem( static_cast<int>( rtimv::colormode::minmaxbox ), "Min/Max Box" );
    m_ui.scaleModeCombo->insertItem( static_cast<int>( rtimv::colormode::user ), "User" );
    m_ui.scaleModeCombo->setCurrentIndex( static_cast<int>( rtimv::colormode::user ) );

    m_ui.scaleTypeCombo->insertItem( static_cast<int>( rtimv::stretch::linear ), "Linear" );
    m_ui.scaleTypeCombo->insertItem( static_cast<int>( rtimv::stretch::log ), "Log" );
    m_ui.scaleTypeCombo->insertItem( static_cast<int>( rtimv::stretch::pow ), "Power" );
    m_ui.scaleTypeCombo->insertItem( static_cast<int>( rtimv::stretch::sqrt ), "Square Root" );
    m_ui.scaleTypeCombo->insertItem( static_cast<int>( rtimv::stretch::square ), "Squared" );

    m_ui.colorbarCombo->insertItem( static_cast<int>( rtimv::colorbar::grey ), "Grey" );
    m_ui.colorbarCombo->insertItem( static_cast<int>( rtimv::colorbar::jet ), "Jet" );
    m_ui.colorbarCombo->insertItem( static_cast<int>( rtimv::colorbar::hot ), "Hot" );
    m_ui.colorbarCombo->insertItem( static_cast<int>( rtimv::colorbar::bone ), "Bone" );
    m_ui.colorbarCombo->insertItem( static_cast<int>( rtimv::colorbar::red ), "Red" );
    m_ui.colorbarCombo->insertItem( static_cast<int>( rtimv::colorbar::green ), "Green" );
    m_ui.colorbarCombo->insertItem( static_cast<int>( rtimv::colorbar::blue ), "Blue" );

    m_ui.pointerViewModecomboBox->insertItem( PointerViewEnabled, "Enabled" );
    m_ui.pointerViewModecomboBox->insertItem( PointerViewOnPress, "On mouse press" );
    m_ui.pointerViewModecomboBox->insertItem( PointerViewDisabled, "Disabled" );
    m_ui.pointerViewModecomboBox->setCurrentIndex( PointerViewOnPress );
}

void rtimvControlPanel::init_panel()
{

    update_panel();

    // initialize the button state for coordinate displays
    showToolTipCoordsChanged( m_imv->showToolTipCoords() );
    showStaticCoordsChanged( m_imv->showStaticCoords() );

    targetXcChanged( m_imv->targetXc() );
    targetYcChanged( m_imv->targetYc() );
    targetVisibleChanged( m_imv->targetVisible() );

    m_ui.imtimerspinBox->setValue( m_imv->imageTimeout() );
}

void rtimvControlPanel::update_panel()
{
    update_ZoomSlider();
    update_ZoomEntry();

    update_xycenEntry();
    update_whEntry();

    update_mindatEntry();
    update_maxdatEntry();
    update_biasEntry();
    update_contrastEntry();

    update_mindatSlider();

    update_maxdatSlider();

    update_biasSlider();

    update_contrastSlider();

    m_ui.scaleTypeCombo->blockSignals( true );
    m_ui.scaleTypeCombo->setCurrentIndex( static_cast<int>( m_imv->stretch() ) );
    m_ui.scaleTypeCombo->blockSignals( false );

    m_ui.scaleModeCombo->blockSignals( true );
    m_ui.scaleModeCombo->setCurrentIndex( static_cast<int>( m_imv->colormode() ) );
    m_ui.scaleModeCombo->blockSignals( false );

    m_ui.colorbarCombo->blockSignals( true );
    m_ui.colorbarCombo->setCurrentIndex( static_cast<int>( m_imv->colorbar() ) );
    m_ui.colorbarCombo->blockSignals( false );
}

void rtimvControlPanel::update_ZoomSlider()
{
    m_ui.ZoomSlider->blockSignals( true );
    m_ui.ZoomSlider->setSliderPosition(
        (int)( ( m_imv->zoomLevel() - m_imv->zoomLevelMin() ) / ( m_imv->zoomLevelMax() - m_imv->zoomLevelMin() ) *
                   ( m_ui.ZoomSlider->maximum() - m_ui.ZoomSlider->minimum() ) +
               .5 ) );
    m_ui.ZoomSlider->blockSignals( false );
}

void rtimvControlPanel::update_ZoomEntry()
{
    char newz[10];
    sprintf( newz, "%4.2f", m_imv->zoomLevel() );

    m_ui.ZoomEntry->blockSignals( true );
    m_ui.ZoomEntry->setText( newz );
    m_ui.ZoomEntry->blockSignals( false );
}

void rtimvControlPanel::on_ZoomSlider_valueChanged( int value )
{
    double zl;
    zl = m_imv->zoomLevelMin() +
         ( (double)value / ( (double)m_ui.ZoomSlider->maximum() - (double)m_ui.ZoomSlider->minimum() ) ) *
             ( m_imv->zoomLevelMax() - m_imv->zoomLevelMin() );
    m_imv->zoomLevel( zl );

    update_ZoomEntry();
}

void rtimvControlPanel::on_Zoom1_clicked()
{
    m_imv->zoomLevel( 1.0 );
    update_ZoomSlider();
}

void rtimvControlPanel::on_Zoom2_clicked()
{
    m_imv->zoomLevel( 2.0 );
    update_ZoomSlider();
}

void rtimvControlPanel::on_Zoom4_clicked()
{
    m_imv->zoomLevel( 4.0 );
    update_ZoomSlider();
}

void rtimvControlPanel::on_Zoom8_clicked()
{
    m_imv->zoomLevel( 8.0 );
    update_ZoomSlider();
}

void rtimvControlPanel::on_Zoom16_clicked()
{
    m_imv->zoomLevel( 16.0 );
    update_ZoomSlider();
}

void rtimvControlPanel::on_ZoomEntry_editingFinished()
{
    m_imv->zoomLevel( m_ui.ZoomEntry->text().toDouble() );
    update_ZoomSlider();
}

void rtimvControlPanel::on_overZoom1_clicked()
{
    m_imv->setPointerOverZoom( 1.0 );
}

void rtimvControlPanel::on_overZoom2_clicked()
{
    m_imv->setPointerOverZoom( 2.0 );
}

void rtimvControlPanel::on_overZoom4_clicked()
{
    m_imv->setPointerOverZoom( 4.0 );
}

void rtimvControlPanel::set_ViewViewMode( int vvm )
{
    if( vvm < 0 || vvm >= ViewViewModeMax )
    {
        m_viewViewMode = ViewViewEnabled;
    }
    else
        m_viewViewMode = vvm;

    if( m_viewViewMode == ViewViewEnabled )
    {
        m_qpmiView = m_qgsView->addPixmap( *( m_imv->getPixmap() ) );
        m_viewBox->setEdgeTol( 5. * m_imv->nx() / m_qgsView->width() );

        m_qpmiView->stackBefore( m_viewLineVert );
    }
    if( m_viewViewMode == ViewViewNoImage && m_qpmiView )
    {
        m_qgsView->removeItem( m_qpmiView );
        delete m_qpmiView;
        m_qpmiView = 0;
    }
}

void ::rtimvControlPanel::update_xycenEntry()
{
    char tmps[10];

    if( !m_ui.xcenEntry->hasFocus() )
    {
        snprintf( tmps, 10, "%.1f", m_imv->get_xcen() );

        m_ui.xcenEntry->blockSignals( true );
        m_ui.xcenEntry->setText( tmps );
        m_ui.xcenEntry->blockSignals( false );
    }
    if( !m_ui.ycenEntry->hasFocus() )
    {
        snprintf( tmps, 10, "%.1f", m_imv->get_ycen() );

        m_ui.ycenEntry->blockSignals( true );
        m_ui.ycenEntry->setText( tmps );
        m_ui.ycenEntry->blockSignals( false );
    }
}

void rtimvControlPanel::update_whEntry()
{
    char tmps[10];
    if( !m_ui.widthEntry->hasFocus() )
    {
        snprintf( tmps, 10, "%.1f", ( (double)m_imv->nx() ) / m_imv->zoomLevel() );

        m_ui.widthEntry->blockSignals( true );
        m_ui.widthEntry->setText( tmps );
        m_ui.widthEntry->blockSignals( false );
    }

    if( !m_ui.heightEntry->hasFocus() )
    {
        snprintf( tmps, 10, "%.1f", ( (double)m_imv->ny() ) / m_imv->zoomLevel() );

        m_ui.heightEntry->blockSignals( true );
        m_ui.heightEntry->setText( tmps );
        m_ui.heightEntry->blockSignals( false );
    }
}

void rtimvControlPanel::on_ViewViewModecheckBox_stateChanged( int st )
{
    enableViewViewMode( st );
}

void rtimvControlPanel::enableViewViewMode( int state )
{
    if( state )
        set_ViewViewMode( ViewViewEnabled );
    else
        set_ViewViewMode( ViewViewNoImage );
}

void rtimvControlPanel::on_xcenEntry_editingFinished()
{
    m_imv->mtxUL_setViewCen( m_ui.xcenEntry->text().toDouble() / m_imv->nx(), m_imv->get_ycen() );
}

void rtimvControlPanel::on_ycenEntry_editingFinished()
{
    m_imv->mtxUL_setViewCen( m_imv->get_xcen(), m_ui.ycenEntry->text().toDouble() / m_imv->ny() );
}

void rtimvControlPanel::on_widthEntry_editingFinished()
{
    double newzoom;
    newzoom = ( (double)m_imv->nx() ) / m_ui.widthEntry->text().toDouble();
    m_imv->zoomLevel( newzoom );
    update_panel();
}

void rtimvControlPanel::on_heightEntry_editingFinished()
{
    double newzoom;
    newzoom = ( (double)m_imv->ny() ) / m_ui.heightEntry->text().toDouble();
    m_imv->zoomLevel( newzoom );
    update_panel();
}

void rtimvControlPanel::on_view_center_clicked()
{
    m_imv->mtxUL_setViewCen( .5, .5 );
}

void rtimvControlPanel::on_view_ul_clicked()
{
    m_imv->mtxUL_setViewCen( m_imv->get_xcen() / m_imv->nx() - 1. / m_imv->zoomLevel(),
                             m_imv->get_ycen() / m_imv->ny() - 1. / m_imv->zoomLevel() );
}

void rtimvControlPanel::on_view_up_clicked()
{
    m_imv->mtxUL_setViewCen( m_imv->get_xcen() / m_imv->nx(),
                             m_imv->get_ycen() / m_imv->ny() - 1. / m_imv->zoomLevel() );
}

void rtimvControlPanel::on_view_ur_clicked()
{
    m_imv->mtxUL_setViewCen( m_imv->get_xcen() / m_imv->nx() + 1. / m_imv->zoomLevel(),
                             m_imv->get_ycen() / m_imv->ny() - 1. / m_imv->zoomLevel() );
}

void rtimvControlPanel::on_view_right_clicked()
{
    m_imv->mtxUL_setViewCen( m_imv->get_xcen() / m_imv->nx() + 1. / m_imv->zoomLevel(),
                             m_imv->get_ycen() / m_imv->ny() );
}

void rtimvControlPanel::on_view_dr_clicked()
{
    m_imv->mtxUL_setViewCen( m_imv->get_xcen() / m_imv->nx() + 1. / m_imv->zoomLevel(),
                             m_imv->get_ycen() / m_imv->ny() + 1. / m_imv->zoomLevel() );
}

void rtimvControlPanel::on_view_down_clicked()
{
    m_imv->mtxUL_setViewCen( m_imv->get_xcen() / m_imv->nx(),
                             m_imv->get_ycen() / m_imv->ny() + 1. / m_imv->zoomLevel() );
}

void rtimvControlPanel::on_view_dl_clicked()
{
    m_imv->mtxUL_setViewCen( m_imv->get_xcen() / m_imv->nx() - 1. / m_imv->zoomLevel(),
                             m_imv->get_ycen() / m_imv->ny() + 1. / m_imv->zoomLevel() );
}

void rtimvControlPanel::on_view_left_clicked()
{
    m_imv->mtxUL_setViewCen( m_imv->get_xcen() / m_imv->nx() - 1. / m_imv->zoomLevel(),
                             m_imv->get_ycen() / m_imv->ny() );
}

void rtimvControlPanel::updateMouseCoords( double x, double y, double v )
{
    if( !m_pointerViewFixed )
    {
        char tmpr[15];
        sprintf( tmpr, "%8.2f", x );

        m_ui.TextCoordX->blockSignals( true );
        m_ui.TextCoordX->setText( tmpr );
        m_ui.TextCoordX->blockSignals( false );

        sprintf( tmpr, "%8.2f", y );

        m_ui.TextCoordY->blockSignals( true );
        m_ui.TextCoordY->setText( tmpr );
        m_ui.TextCoordY->blockSignals( false );

        sprintf( tmpr, "%8.2f", v );

        m_ui.TextPixelVal->blockSignals( true );
        m_ui.TextPixelVal->setText( tmpr );
        m_ui.TextPixelVal->blockSignals( false );

        if( m_pointerViewEmpty && m_pointerViewMode == PointerViewEnabled )
        {
            m_ui.pointerView->setScene( m_imv->get_qgs() );
            m_pointerViewEmpty = false;
        }

        m_ui.pointerView->centerOn( x, y );
    }
}

void rtimvControlPanel::nullMouseCoords()
{
    if( !m_pointerViewFixed )
    {
        m_ui.pointerView->setScene( m_qgsEmpty );
        m_pointerViewEmpty = true;

        m_ui.TextCoordX->blockSignals( true );
        m_ui.TextCoordX->setText( "" );
        m_ui.TextCoordX->blockSignals( false );

        m_ui.TextCoordY->blockSignals( true );
        m_ui.TextCoordY->setText( "" );
        m_ui.TextCoordY->blockSignals( false );

        m_ui.TextPixelVal->blockSignals( true );
        m_ui.TextPixelVal->setText( "" );
        m_ui.TextPixelVal->blockSignals( false );
    }
}

void rtimvControlPanel::viewLeftPressed( QPointF mp )
{
    static_cast<void>( mp );

    if( m_pointerViewMode == PointerViewOnPress )
    {
        m_ui.pointerView->setScene( m_imv->get_qgs() );
        m_pointerViewEmpty = false;
        if( m_pointerViewFixed )
        {
            m_ui.pointerView->centerOn( m_pointerViewCen.x(), m_pointerViewCen.y() );
        }
    }
}

void rtimvControlPanel::viewLeftClicked( QPointF mp )
{
    if( m_pointerViewMode == PointerViewOnPress )
    {
        m_ui.pointerView->setScene( m_qgsEmpty );
        m_pointerViewEmpty = true;
    }
    if( m_pointerViewWaiting )
    {
        set_pointerViewCen( mp );
    }
}

void rtimvControlPanel::set_PointerViewMode( int pvm )
{
    if( pvm < 0 || pvm >= PointerViewModeMax || pvm == PointerViewEnabled )
    {
        m_pointerViewMode = PointerViewEnabled;
        m_ui.pointerView->setScene( m_imv->get_qgs() );
        m_pointerViewEmpty = false;
    }
    if( pvm == PointerViewOnPress )
    {
        m_pointerViewMode = PointerViewOnPress;
        m_ui.pointerView->setScene( m_qgsEmpty );
        m_pointerViewEmpty = true;
    }
    if( pvm == PointerViewDisabled )
    {
        m_pointerViewMode = PointerViewDisabled;
        m_ui.pointerView->setScene( m_qgsEmpty );
        m_pointerViewEmpty = true;
    }
}

void rtimvControlPanel::on_pointerViewModecomboBox_activated( int pvm )
{
    set_PointerViewMode( pvm );
}

void rtimvControlPanel::on_pointerSetLocButton_clicked()
{
    if( !m_pointerViewFixed )
    {
        m_pointerViewWaiting = true;
        m_ui.pointerSetLocButton->setText( "Waiting" );
    }
    else
    {
        m_ui.pointerSetLocButton->setText( "Set Location" );
        m_pointerViewWaiting = false;
        m_pointerViewFixed = false;
    }
}

void rtimvControlPanel::set_pointerViewCen( QPointF mp )
{
    std::cerr << mp.x() << " " << mp.y() << "\n";

    m_pointerViewCen = mp;
    m_ui.pointerSetLocButton->setText( "Clear Location" );
    m_pointerViewWaiting = false;
    m_pointerViewFixed = true;
}

void rtimvControlPanel::on_scaleTypeCombo_activated( int ct )
{
    m_imv->stretch( static_cast<rtimv::stretch>( ct ) );

    m_imv->mtxUL_recolor();
}

void rtimvControlPanel::on_colorbarCombo_activated( int cb )
{
    m_imv->mtxUL_load_colorbar( static_cast<rtimv::colorbar>( cb ), true );
}

void rtimvControlPanel::update_mindatSlider()
{
    int pos;

    pos = (int)( m_ui.mindatSlider->minimum() +
                 ( m_imv->minScaleData() - m_imv->minImageData() ) / ( m_imv->maxImageData() - m_imv->minImageData() ) *
                     ( m_ui.mindatSlider->maximum() - m_ui.mindatSlider->minimum() ) +
                 .5 );

    m_ui.mindatSlider->blockSignals( true );
    m_ui.mindatSlider->setSliderPosition( pos );
    m_ui.mindatSlider->blockSignals( false );
}

void rtimvControlPanel::update_mindatEntry()
{
    char tmps[15];
    if( !m_ui.mindatEntry->hasFocus() )
    {
        sprintf( tmps, "%14.1f", m_imv->minScaleData() );

        m_ui.mindatEntry->blockSignals( true );
        m_ui.mindatEntry->setText( tmps );
        m_ui.mindatEntry->blockSignals( false );
    }
}

void rtimvControlPanel::update_mindatRelEntry()
{
    char tmps[15];
    if( !m_ui.mindatRelEntry->hasFocus() )
    {
        sprintf( tmps,
                 "%5.3f",
                 ( m_imv->minScaleData() - m_imv->minImageData() ) /
                     ( m_imv->maxImageData() - m_imv->minImageData() ) );

        m_ui.mindatRelEntry->blockSignals( true );
        m_ui.mindatRelEntry->setText( tmps );
        m_ui.mindatRelEntry->blockSignals( false );
    }
}

void rtimvControlPanel::update_maxdatSlider()
{
    if( !m_ui.maxdatSlider->hasFocus() )
    {
        int pos;
        pos = (int)( m_ui.maxdatSlider->minimum() +
                     ( m_imv->maxScaleData() - m_imv->minImageData() ) /
                         ( m_imv->maxImageData() - m_imv->minImageData() ) *
                         ( m_ui.maxdatSlider->maximum() - m_ui.maxdatSlider->minimum() ) +
                     .5 );

        m_ui.maxdatSlider->blockSignals( true );
        m_ui.maxdatSlider->setSliderPosition( pos );
        m_ui.maxdatSlider->blockSignals( false );
    }
}

////

void rtimvControlPanel::update_maxdatEntry()
{
    char tmps[15];
    if( !m_ui.maxdatEntry->hasFocus() )
    {
        sprintf( tmps, "%14.1f", m_imv->maxScaleData() );

        m_ui.maxdatEntry->blockSignals( true );
        m_ui.maxdatEntry->setText( tmps );
        m_ui.maxdatEntry->blockSignals( false );
    }
}

void rtimvControlPanel::update_maxdatRelEntry()
{
    char tmps[15];
    if( !m_ui.maxdatRelEntry->hasFocus() )
    {
        sprintf( tmps,
                 "%5.3f",
                 ( m_imv->maxScaleData() - m_imv->minImageData() ) /
                     ( m_imv->maxImageData() - m_imv->minImageData() ) );

        m_ui.maxdatRelEntry->blockSignals( true );
        m_ui.maxdatRelEntry->setText( tmps );
        m_ui.maxdatRelEntry->blockSignals( false );
    }
}

void rtimvControlPanel::update_biasSlider()
{
    if( !m_ui.biasSlider->hasFocus() )
    {
        int pos;
        pos = (int)( m_ui.biasSlider->minimum() +
                     ( .5 * ( m_imv->maxScaleData() + m_imv->minScaleData() ) - m_imv->minImageData() ) /
                         ( m_imv->maxImageData() - m_imv->minImageData() ) *
                         ( m_ui.biasSlider->maximum() - m_ui.biasSlider->minimum() ) +
                     .5 );

        m_ui.biasSlider->blockSignals( true );
        m_ui.biasSlider->setSliderPosition( pos );
        m_ui.biasSlider->blockSignals( false );
    }
}

void rtimvControlPanel::update_biasEntry()
{
    char tmps[15];
    if( !m_ui.biasEntry->hasFocus() )
    {
        sprintf( tmps, "%14.1f", 0.5 * ( m_imv->maxScaleData() + m_imv->minScaleData() ) );

        m_ui.biasEntry->blockSignals( true );
        m_ui.biasEntry->setText( tmps );
        m_ui.biasEntry->blockSignals( false );
    }
}

void rtimvControlPanel::update_biasRelEntry()
{
    char tmps[15];
    if( !m_ui.biasRelEntry->hasFocus() )
    {
        sprintf( tmps,
                 "%5.3f",
                 ( 0.5 * ( m_imv->maxScaleData() + m_imv->minScaleData() ) - m_imv->minImageData() ) /
                     ( m_imv->maxImageData() - m_imv->minImageData() ) );

        m_ui.biasRelEntry->blockSignals( true );
        m_ui.biasRelEntry->setText( tmps );
        m_ui.biasRelEntry->blockSignals( false );
    }
}

void rtimvControlPanel::update_contrastSlider()
{
    int pos;
    pos = (int)( m_ui.contrastSlider->minimum() +
                 ( m_imv->maxScaleData() - m_imv->minScaleData() ) / ( m_imv->maxImageData() - m_imv->minImageData() ) *
                     ( m_ui.biasSlider->maximum() - m_ui.biasSlider->minimum() ) +
                 .5 );

    m_ui.contrastSlider->blockSignals( true );
    m_ui.contrastSlider->setSliderPosition( pos );
    m_ui.contrastSlider->blockSignals( false );
}

void rtimvControlPanel::update_contrastEntry()
{
    char tmps[15];
    if( !m_ui.contrastEntry->hasFocus() )
    {
        sprintf( tmps, "%14.1f", ( m_imv->maxScaleData() - m_imv->minScaleData() ) );

        m_ui.contrastEntry->blockSignals( true );
        m_ui.contrastEntry->setText( tmps );
        m_ui.contrastEntry->blockSignals( false );
    }
}

void rtimvControlPanel::update_contrastRelEntry()
{
    char tmps[15];
    if( !m_ui.contrastRelEntry->hasFocus() )
    {
        sprintf( tmps,
                 "%5.1f",
                 m_imv->maxImageData() - m_imv->minImageData() / ( m_imv->maxScaleData() - m_imv->minScaleData() ) );

        m_ui.contrastRelEntry->blockSignals( true );
        m_ui.contrastRelEntry->setText( tmps );
        m_ui.contrastRelEntry->blockSignals( false );
    }
}

void rtimvControlPanel::on_scaleModeCombo_activated( int index )
{
    if( static_cast<rtimv::colormode>( index ) == rtimv::colormode::minmaxglobal )
    {
        m_imv->mtxUL_colormode( rtimv::colormode::minmaxglobal );
    }
    else if( static_cast<rtimv::colormode>( index ) == rtimv::colormode::user )
    {
        m_imv->mtxUL_colormode( rtimv::colormode::user );
    }
    else if( static_cast<rtimv::colormode>( index ) == rtimv::colormode::minmaxbox )
    {
        m_imv->toggleColorBoxOn();
    }
    else
    {
        m_imv->mtxUL_colormode( rtimv::colormode::minmaxglobal );
    }
}

void rtimvControlPanel::on_mindatSlider_valueChanged( int value )
{
    double sc = ( (double)( value - m_ui.mindatSlider->minimum() ) ) /
                ( (double)( m_ui.mindatSlider->maximum() - m_ui.mindatSlider->minimum() ) );

    m_imv->minScaleData( m_imv->minImageData() + ( m_imv->maxImageData() - m_imv->minImageData() ) * sc );

    m_imv->mtxUL_colormode( rtimv::colormode::user );
}

void rtimvControlPanel::on_mindatEntry_editingFinished()
{
    m_imv->minScaleData( m_ui.mindatEntry->text().toDouble() );

    m_imv->mtxUL_colormode( rtimv::colormode::user );
}

void rtimvControlPanel::on_maxdatSlider_valueChanged( int value )
{
    double sc = ( (double)( value - m_ui.maxdatSlider->minimum() ) ) /
                ( (double)( m_ui.maxdatSlider->maximum() - m_ui.maxdatSlider->minimum() ) );

    m_imv->maxScaleData( m_imv->minImageData() + ( m_imv->maxImageData() - m_imv->minImageData() ) * sc );

    m_imv->mtxUL_colormode( rtimv::colormode::user );
}

void rtimvControlPanel::on_maxdatEntry_editingFinished()
{
    m_imv->maxScaleData( m_ui.maxdatEntry->text().toDouble() );

    m_imv->mtxUL_colormode( rtimv::colormode::user );
}

void rtimvControlPanel::on_biasSlider_valueChanged( int value )
{
    double bias = ( (double)( value - m_ui.biasSlider->minimum() ) ) /
                  ( (double)( m_ui.biasSlider->maximum() - m_ui.biasSlider->minimum() ) );

    m_imv->bias_rel( bias );

    m_imv->mtxUL_colormode( rtimv::colormode::user );
}

void rtimvControlPanel::on_biasEntry_editingFinished()
{
    m_imv->bias( m_ui.biasEntry->text().toDouble() );

    m_imv->mtxUL_colormode( rtimv::colormode::user );
}

void rtimvControlPanel::on_contrastSlider_valueChanged( int value )
{
    double cont = ( (double)( value - m_ui.contrastSlider->minimum() ) ) /
                  ( (double)( m_ui.contrastSlider->maximum() - m_ui.contrastSlider->minimum() ) );

    cont = cont * ( m_imv->maxImageData() - m_imv->minImageData() );

    m_imv->contrast( cont );

    m_imv->mtxUL_colormode( rtimv::colormode::user );
}

void rtimvControlPanel::on_contrastEntry_editingFinished()
{
    m_imv->contrast( m_ui.contrastEntry->text().toDouble() );

    m_imv->mtxUL_colormode( rtimv::colormode::user );
}

void rtimvControlPanel::on_imtimerspinBox_valueChanged( int to )
{
    m_imv->imageTimeout( to );
}

void rtimvControlPanel::on_statsBoxButton_clicked()
{
    if( m_statsBoxButtonState )
    {
        emit hideStatsBox();
        m_ui.statsBoxButton->setText( "Show Stats Box" );
        m_statsBoxButtonState = false;
    }
    else
    {
        emit launchStatsBox();
        m_ui.statsBoxButton->setText( "Hide Stats Box" );
        m_statsBoxButtonState = true;
    }
}

void rtimvControlPanel::viewBoxMoved( const QRectF &vbr )
{
    // QPointF newcenV = m_ui.viewView->mapFromScene(newcen);
    // double viewZoom = (double)m_ui.viewView->width()/(double)m_imv->nx();
    // QRectF vbr = m_viewBox->rect();

    QPointF np = m_qpmiView->mapFromItem( m_viewBox, QPointF( vbr.x(), vbr.y() ) );
    QPointF np2 = m_qpmiView->mapFromItem( m_viewBox, QPointF( vbr.bottom(), vbr.right() ) );

    double nxcen = np.x() + .5 * ( np2.x() - np.x() );
    double nycen = np.y() + .5 * ( np2.y() - np.y() );

    m_imv->mtxUL_setViewCen( nxcen / (double)m_imv->nx(), nycen / (double)m_imv->nx(), true );
}

void rtimvControlPanel::showToolTipCoordsChanged( bool sttc )
{
    if( sttc )
    {
        m_ui.toolTipCoordsButton->setText( "Hide Pointer Coords" );
    }
    else
    {
        m_ui.toolTipCoordsButton->setText( "Show Pointer Coords" );
    }
}

void rtimvControlPanel::showStaticCoordsChanged( bool ssc )
{
    if( ssc )
    {
        m_ui.staticCoordsButton->setText( "Hide Static Coords" );
    }
    else
    {
        m_ui.staticCoordsButton->setText( "Show Static Coords" );
    }
}

void rtimvControlPanel::on_toolTipCoordsButton_clicked()
{
    // Toggle
    emit showToolTipCoords( !m_imv->showToolTipCoords() );
}

void rtimvControlPanel::on_staticCoordsButton_clicked()
{
    // Toggle
    emit showStaticCoords( !m_imv->showStaticCoords() );
}

void rtimvControlPanel::targetXcChanged( float txc )
{
    char str[16];
    snprintf( str, sizeof( str ), "%0.3f", txc );
    m_ui.lineEditTargetFractionX->setText( str );

    snprintf( str, sizeof( str ), "%0.2f", txc * ( m_imv->nx() ) - 0.5 );
    m_ui.lineEditTargetPixelX->setText( str );
}

void rtimvControlPanel::targetYcChanged( float tyc )
{
    char str[16];
    snprintf( str, sizeof( str ), "%0.3f", tyc );
    m_ui.lineEditTargetFractionY->setText( str );

    snprintf( str, sizeof( str ), "%0.2f", tyc * ( m_imv->ny() ) - 0.5 );
    m_ui.lineEditTargetPixelY->setText( str );
}

void rtimvControlPanel::targetVisibleChanged( bool tv )
{
    if( tv )
    {
        m_ui.buttonTargetCross->setText( "hide" );
    }
    else
    {
        m_ui.buttonTargetCross->setText( "show" );
    }
}

void rtimvControlPanel::on_buttonTargetCross_clicked()
{
    // toggle
    emit targetVisible( !m_imv->targetVisible() );
}

void rtimvControlPanel::on_lineEditTargetPixelX_returnPressed()
{
    float txc = -1;
    bool ok = false;
    try
    {
        txc = m_ui.lineEditTargetPixelX->text().toFloat( &ok );
    }
    catch( ... )
    {
        return;
    }

    if( txc == -1 || ok == false )
        return;

    txc = ( txc + 0.5 ) / m_imv->nx();

    emit targetXc( txc );
}

void rtimvControlPanel::on_lineEditTargetPixelY_returnPressed()
{
    float tyc = -1;
    bool ok = false;
    try
    {
        tyc = m_ui.lineEditTargetPixelY->text().toFloat( &ok );
    }
    catch( ... )
    {
        return;
    }

    if( tyc == -1 || ok == false )
        return;

    tyc = ( tyc + 0.5 ) / m_imv->ny();

    emit targetYc( tyc );
}

void rtimvControlPanel::on_lineEditTargetFractionX_returnPressed()
{
    float txc = -1;
    bool ok = false;
    try
    {
        txc = m_ui.lineEditTargetFractionX->text().toFloat( &ok );
    }
    catch( ... )
    {
        return;
    }

    if( txc == -1 || ok == false )
        return;

    emit targetXc( txc );
}

void rtimvControlPanel::on_lineEditTargetFractionY_returnPressed()
{
    float tyc = -1;
    bool ok = false;
    try
    {
        tyc = m_ui.lineEditTargetFractionY->text().toFloat( &ok );
    }
    catch( ... )
    {
        return;
    }

    if( tyc == -1 || ok == false )
        return;

    emit targetYc( tyc );
}
