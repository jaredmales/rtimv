/** \file circleTimeSeries.hpp
 * \author Jared R. Males (jaredmales@gmail.com)
 * \brief Declaration of a circular-buffer time-series class
 *
 */

#ifndef rtimv_circleTimeSeries_hpp
#define rtimv_circleTimeSeries_hpp

#include <qwt/qwt_series_data.h>

class circleTimeSeries : public QwtPointSeriesData
{
  public:
    circleTimeSeries( size_t sz, bool useage )
    {
        set_length( sz );
        cursor = -1;
        _size = 0;
        useAge = useage;
        minY = 1e99;
        maxY = 0;
    }

    // virtual QwtPointSeriesData * copy() const;

    std::vector<double> t;
    std::vector<double> Y;
    std::vector<double> age;

    void set_length( size_t l )
    {
        t.resize( l );
        Y.resize( l );
        age.resize( l );
    }

    size_t get_length() const
    {
        return t.size();
    }

    int cursor;
    size_t _size;
    virtual size_t size() const
    {
        return _size;
    }

    double minY;
    double maxY;

    bool useAge;

    void add_point( double nt, double ny )
    {
        double poppedY;
        double dt;
        size_t _i;

        if( _size < get_length() )
        {
            t[_size] = nt;
            Y[_size] = ny;

            age[_size] = 0;
            if( _size > 0 )
            {
                dt = t[_size] - t[_size - 1];
                for( size_t i = 0; i < _size; i++ )
                {
                    age[i] += dt;
                }
            }

            _size++;
            cursor++;
            if( (size_t)cursor >= get_length() )
                cursor = 0; // cast to size_t works b/c cursor++ means it can't be -1.

            if( ny < minY )
                minY = ny;
            if( ny > maxY )
                maxY = ny;
        }
        else
        {
            size_t next = cursor + 1;
            if( next >= get_length() )
                next = 0;

            poppedY = Y[next];

            t[next] = nt;
            Y[next] = ny;

            dt = t[next] - t[cursor];

            cursor = next;
            age[cursor] = 0;
            for( size_t i = 0; i < get_length(); i++ )
                age[sample_i( i )] += dt;

            if( poppedY == maxY )
            {
                maxY = 0;
                for( size_t i = 0; i < get_length(); i++ )
                {
                    _i = sample_i( i );
                    if( Y[_i] > maxY )
                        maxY = Y[_i];
                }
            }
            if( poppedY == minY )
            {
                minY = 1e99;
                for( size_t i = 0; i < get_length(); i++ )
                {
                    _i = sample_i( i );
                    if( Y[_i] < minY )
                        minY = Y[_i];
                }
            }
        }
    }

    size_t sample_i( size_t i ) const
    {
        size_t _i = cursor + 1 + i;
        if( _i >= _size )
            _i -= _size;
        return _i;
    }

    void clear()
    {
        cursor = -1;
        _size = 0;
        minY = 1e99;
        maxY = 0;
    }

    virtual double x( size_t i ) const
    {

        if( useAge )
        {
            return age[sample_i( i )];
        }

        else
            return t[sample_i( i )];
    }

    virtual double y( size_t i ) const
    {
        return Y[sample_i( i )];
    }

    virtual QPointF sample( size_t i ) const
    {
        QPointF p;
        p.setX( x( i ) );
        p.setY( y( i ) );

        return p;
    }

    virtual QRectF boundingRect() const
    {
        if( useAge )
        {
            return QRectF( 0, minY, age[sample_i( 0 )], maxY - minY );
        }
        else
        {
            return QRectF( t[cursor], minY, t[cursor] - t[sample_i( 0 )], maxY - minY );
        }
    }
};

#endif // rtimv_circleTimeSeries_hpp
