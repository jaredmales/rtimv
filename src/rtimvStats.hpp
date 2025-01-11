
#ifndef rtimv_rtimvStats_hpp
#define rtimv_rtimvStats_hpp

#include <cstdio>
#include <unistd.h>

#include <mutex>

#include <QDialog>
#include <QThread>

#include <mx/improc/eigenImage.hpp>

#include "ui_imviewerStats.h"

#include "rtimvBase.hpp"

class rtimvStats;

/// Thread class to start the stats thread.
class StatsThread : public QThread
{
  public:
    void run();
    rtimvStats *m_imvs;
};

/// Class to manage calculating statistics in the designated image region
/** Copies the data in the region to a local array.  While less efficient than not doing so,
 * this is done to avoid problems if the image goes away while the stats calculations are occuring.
 */
class rtimvStats : public QDialog
{
    Q_OBJECT

  private:
    rtimvStats()
    {
    }

  public:
    /// Constructor
    rtimvStats( rtimvBase *imv, ///< [in] The rtimv instance this is connected to
                std::shared_mutex *calMutex,
                QWidget *Parent = nullptr,            ///< [in] [optional] Qt parent widget
                Qt::WindowFlags f = Qt::WindowFlags() ///< [in] [optional] Qt flags for this widget
    );

    /// Destructor
    ~rtimvStats();

  protected:
    rtimvBase *m_imv{ nullptr }; ///< The rtimv instance this is connected to

    std::shared_mutex *m_calMutex{ nullptr };

    int m_statsPause{ 20 }; ///< Pause between checks if the stats thread needs to calculate, milliseconds.

    QTimer m_updateTimer;           ///< When this times out the GUI is updated if needed.
    int m_updateTimerTimeout{ 50 }; ///< The GUI update timeout, milliseconds.

    size_t m_nx{ 0 }; ///< The x size of the image, stored internally for copying from image data to local array
    size_t m_ny{ 0 }; ///< The y size of the image, stored internally for copying from image data to local array
    size_t m_x0{ 0 }; ///< The x coordinate of the starting corner of the region, stored internally for copying from
                      ///< image data to local array
    size_t m_x1{ 0 }; ///< The x coordinate of the ending corner of the region, stored internally for copying from image
                      ///< data to local array
    size_t m_y0{ 0 }; ///< The y coordinate of the starting corner of the region, stored internally for copying from
                      ///< image data to local array
    size_t m_y1{ 0 }; ///< The y coordinate of the ending corner of the region, stored internally for copying from image
                      ///< data to local array

    mx::improc::eigenImage<float> m_imdata; ///< Local image storage.  The region is copied to this for robustness
                                            ///< against segfaults from image changes.

    float m_dataMin{ 0 };  ///< The minimum value in the data
    float m_dataMax{ 0 };  ///< The maximum value in the data
    float m_dataMean{ 0 }; ///< The mean value in the data

    std::vector<float> m_medWork; ///< Working memory for median calculation

    float m_dataMedian{ 0 }; ///< The median value in the data

    int m_regionChanged{
        0 };                 ///< Flag indicating that the region has changed, either size, location, or the data itself
    int m_statsChanged{ 0 }; ///< Flag indicating that the statistics have changed
    int m_dieNow{ 0 };       ///< Flag indicating that the stats thread should exit

    std::mutex m_dataMutex; ///< Mutex for updating the local image (m_imdata).

    StatsThread m_statsThread; ///< Thread for calculating the stats.

  public:
    /// Called by rtimvMainWindow to indicate that the data has changed.
    void setImdata();

    /// Called by rtimvMainWindow to indicate that the region has changed.
    /**
     * The calibration data mutex must be locked when this is called.
     */
    void mtxL_setImdata( size_t nx, ///< [in] the new image x size
                         size_t ny, ///< [in] the new image y size
                         size_t x0, ///< The x coordinate of the starting corner of the region.
                         size_t x1, ///< The x coordinate of the ending corner of the region.
                         size_t y0, ///< The y coordinate of the starting corner of the region.
                         size_t y1, ///< The y coordinate of the ending corner of the region.
                         std::shared_lock<std::shared_mutex> &lock ///< The locked mutex.
    );

    /// Run funciton for the statistics thread.
    /** Pauses for m_statsPause then calls calcStats, until m_dieNow is called.
     */
    void statsThread();

    /// Calculate the statistics.
    /**
     * Both the rtimv cal mutex and the local data mutex will be try-locked.
     */
    void mtxUL_calcStats();

  protected slots:

    /// Update the GUI.
    /** Called when m_updateTimer times out.
     */
    void updateGUI();

  private:
    Ui::statsform ui;
};

#endif // rtimv_rtimvStats_hpp
