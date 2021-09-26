#ifndef GUL_IMAGE_H
#define GUL_IMAGE_H

#include<iostream>
#include<string.h>
#include<cassert>
#include<type_traits>
#include<functional>
#include<cstdint>
#include<cmath>


namespace gul
{

inline uint8_t mix( uint8_t  a, uint8_t  b, float t)
{
    return static_cast<uint8_t>( (float(a) * (1.0f-t) + float(b)*t));

}

/**
 * @brief The channel1f struct
 *
 * Essentially a 1D image of floating point values.
 * Mostly used for intermediate stages;
 */
struct channel1f
{
      std::vector<float> data;

      channel1f(uint32_t w, uint32_t h) : _width(w), _height(h)
      {
          data.resize(w*h);
      }
      float & operator()(uint32_t u, uint32_t v)
      {
          return data[v*_width+u];
      }
      float const & operator()(uint32_t u, uint32_t v) const
      {
          return data[v*_width+u];
      }

      uint32_t width() const
      {
          return _width;
      }

      uint32_t height() const
      {
          return _height;
      }


private:
      uint32_t _width;
      uint32_t _height;
};


struct ColorChannel
{
      using channel_type = ColorChannel;
      uint32_t stride = 0;
      uint32_t offset = 0;

      ColorChannel()
      {

      }
      ColorChannel(uint8_t * data, uint32_t _offset, uint32_t _stride, uint32_t w, uint32_t h) : stride(_stride), offset(_offset), width(w), height(h), ptr(data)
      {
      }

      void reset(uint8_t * data, uint32_t _offset, uint32_t _stride, uint32_t w, uint32_t h)
      {
            stride = _stride;
            offset = _offset;
            width  = w;
            height = h;
            ptr = data;
      }


      uint8_t & operator()(uint32_t u, uint32_t v)
      {
          const auto rowLength = stride * width;
          return ptr[  v*rowLength + u*stride + offset ];
          //return static_cast<uint8_t*>(static_cast<void*>(&ptr[v*width+u]))[offset];
      }
      uint8_t const & operator()(uint32_t u, uint32_t v) const
      {
          const auto rowLength = stride * width;
          return ptr[  v*rowLength + u*stride + offset ];
          //return static_cast<uint8_t*>(static_cast<void*>(&ptr[v*width+u]))[offset];
      }

      ColorChannel& operator=( ColorChannel const & other)
      {
          if( width  !=  other.width ||
              height !=  other.height)
          {
              throw std::logic_error("Channels are of different size");
          }

          auto w = other.getWidth();
          auto h = other.getHeight();

          for(uint32_t j=0;j<h;j++)
          {
            for(uint32_t i=0;i<w;i++)
            {
                (*this)(i,j) = other(i,j);
            }
          }

          return *this;
      }

      ColorChannel& operator=( uint8_t val)
      {
          auto w = getWidth();
          auto h = getHeight();

          for(uint32_t j=0;j<h;j++)
          {
            for(uint32_t i=0;i<w;i++)
            {
                (*this)(i,j) = val;
            }
          }

          return *this;
      }

      ColorChannel& operator=( int val )
      {
          return this->operator=( static_cast<uint8_t>(val) );
      }

      ColorChannel& operator=( float val )
      {
          return this->operator=( static_cast<uint8_t>(val*255.0f) );
      }

      ColorChannel& operator=( channel1f && val )
      {
          for(uint32_t j = 0; j < getHeight(); j++)
          {
                for(uint32_t i = 0; i < getWidth(); i++)
                {
                    (*this)(i,j) = static_cast<uint8_t>(255.0f * val(i,j));
                }
          }

          return *this;
      }

      ColorChannel& operator=( channel1f const &val )
      {
          for(uint32_t j = 0; j < getHeight(); j++)
          {
                for(uint32_t i = 0; i < getWidth(); i++)
                {
                    (*this)(i,j) = static_cast<uint8_t>(255.0f * val(i,j));
                }
          }
          return *this;
      }

      channel1f operator+( ColorChannel const & other) const
      {
          channel1f R(width,height);

          auto & b1 = *this;
          auto & b2 = other;


          const float sc = 1.0f / 255.0f;
          for(uint32_t j = 0; j < getHeight(); j++)
          {
                for(uint32_t i = 0; i < getWidth(); i++)
                {
                    R(i,j) =( static_cast<float>( b1(i,j) ) + static_cast<float>(b2(i,j))) * sc;
                }
          }

          return R;
      }

      channel1f operator*( ColorChannel const & other) const
      {
          channel1f R(width,height);

          auto & b1 = *this;
          auto & b2 = other;

          const float sc = 1.0f / (255.0f*255.0f);

          for(uint32_t j = 0; j < getHeight(); j++)
          {
                for(uint32_t i = 0; i < getWidth(); i++)
                {
                    R(i,j) = ( static_cast<float>( b1(i,j) ) * static_cast<float>(b2(i,j))) * sc;
                }
          }

          return R;
      }

      template<typename Callable_t>
      void apply( Callable_t C)
      {
          float sw = 1.0f / float(width);
          float sh = 1.0f / float(height);

          for(uint32_t v = 0; v < height; ++v)
          for(uint32_t u = 0; u < width; ++u)
          {
              float x = static_cast<float>(u) * sw;
              float y = static_cast<float>(v) * sh;

              (*this)(u,v) =  static_cast<uint8_t>( C(x,y) * 255);
          }
      }

      template<typename Callable_t>
      ColorChannel& operator=( Callable_t C )
      {
          apply(C);
          return *this;
      }

      uint32_t getWidth() const
      {
          return width;
      }
      uint32_t getHeight() const
      {
          return height;
      }
      uint32_t getStride() const
      {
          return stride;
      }
    private:
      uint32_t  width = 0;
      uint32_t  height= 0;
      uint8_t   *ptr =nullptr;

      friend class Image;
};


inline channel1f operator * (ColorChannel const & a, float b)
{
    channel1f D( a.getWidth(), a.getHeight());

    auto *Y = &a(0,0);

    const float sc = 1.0f/255.0f;

    for(auto & v : D.data)
    {
        v = sc * *Y * b;
        Y+=4;
    }
    return D;
}

inline channel1f operator * (float b, ColorChannel const & a)
{
    return operator*(a,b);
}



inline channel1f operator + (ColorChannel const & a, float b)
{
    //std::cout << "construct OneChannel+float" << std::endl;
    channel1f D(a.getWidth(),a.getHeight());

    auto *Y = &a(0,0);

    float sc = 1.0f/255.0f;

    for(auto & v : D.data)
    {
        v = *Y*sc + b;
        Y+=4;
    }
    return D;
}


inline channel1f operator + (float b, ColorChannel const & a )
{
    return operator+(a,b);
}


inline channel1f&& operator + (channel1f && D, channel1f && E)
{
    //std::cout << "move OneChannel+OneChannel" << std::endl;
    auto b = E.data.begin();
    for(auto & v : D.data)
    {
        v += *b++;
    }
    return std::move(D);
}

inline channel1f&& operator + (channel1f && D, float b)
{
    for(auto & v : D.data)
    {
        v += b;
    }
    return std::move(D);
}

inline channel1f&& operator - (channel1f && D, float b)
{
    return operator+( std::move(D),-b);
}



inline channel1f operator - (ColorChannel const & a, float b)
{
    return operator+(a,-b);
}


inline channel1f operator - (float b, ColorChannel const & a )
{
    //std::cout << "Construct OneChannel+float" << std::endl;
    channel1f D(a.getWidth(),a.getHeight());

    auto *Y = &a(0,0);

    const float sc = 1.0f / 255.0f;

    for(auto & v : D.data)
    {
        v = b - *Y*sc;
        Y+=4;
    }
    return D;
}

class Image
{
public:

    Image() : Image(8,8,4)
    {
    }
    explicit Image(uint32_t w) : Image(w,w,4)
    {
    }
    Image(uint32_t w, uint32_t h, uint32_t ch=4)
    {
        resize(w,h,ch);
    }

    Image(const Image & other) : Image()
    {
        resize(other.getWidth(), other.getHeight(), other.getChannels());
        m_data = other.m_data;
        _setChannels( other.getWidth(), other.getHeight(), other.getChannels() );
    }

    Image(Image && other) : Image(1,1,4)
    {
        m_data = std::move(other.m_data);
        _setChannels( other.getWidth(), other.getHeight(), other.getChannels() );
        other.m_width = other.m_height = other.m_channels = 0;
    }

    Image& operator=(const Image & other)
    {
        if( &other != this)
        {
            resize(other.getWidth(), other.getHeight(), other.getChannels());

            auto & out = *this;
            for(uint32_t j=0;j<out.getHeight();j++)
            {
                for(uint32_t i=0;i<out.getWidth();i++)
                {
                    for(uint32_t c =0; c < out.getChannels(); c++)
                    {
                        out(i,j,c) = other( i, j, c );
                    }
                }
            }
        }
        return *this;
    }
    Image& operator=(Image && other)
    {
        if( &other != this)
        {
            m_data = std::move(other.m_data);
            _setChannels( other.getWidth(), other.getHeight(), other.getChannels() );
            other.m_width = other.m_height = other.m_channels = 0;
        }
        return *this;
    }

    void resize(uint32_t w, uint32_t h, uint32_t channels=4)
    {
        assert( channels <= 4);
        if( w*h*channels != m_data.size())
        {
            m_data.resize(w*h*channels);
        }

        m_channels = channels;

        _setChannels(w,h,channels);
    }

    void _setChannels(uint32_t width, uint32_t height, uint32_t channels)
    {
        assert( channels <= 4 && channels >= 1);
        m_channels = channels;
        m_width = width;
        m_height = height;
        if( channels == 4)
        {
            a.reset( m_data.data(), 3, 4, width, height);
            b.reset( m_data.data(), 2, 4, width, height);
            g.reset( m_data.data(), 1, 4, width, height);
            r.reset( m_data.data(), 0, 4, width, height);
        }
        if( channels == 3)
        {
            a.reset( m_data.data(), 2, 3, width, height);
            b.reset( m_data.data(), 2, 3, width, height);
            g.reset( m_data.data(), 1, 3, width, height);
            r.reset( m_data.data(), 0, 3, width, height);
        }
        if( channels == 2)
        {
            a.reset( m_data.data(), 1, 2, width, height);
            b.reset( m_data.data(), 1, 2, width, height);
            g.reset( m_data.data(), 1, 2, width, height);
            r.reset( m_data.data(), 0, 2, width, height);
        }
        if( channels == 1)
        {
            a.reset( m_data.data(), 0, 1, width, height);
            b.reset( m_data.data(), 0, 1, width, height);
            g.reset( m_data.data(), 0, 1, width, height);
            r.reset( m_data.data(), 0, 1, width, height);
        }
    }

    void copyFromBuffer(void const * src, uint32_t totalBytes, uint32_t width, uint32_t height, uint32_t ch=4)
    {
          assert(totalBytes%sizeof(uint32_t)==0);

          resize(width,height,ch);
          memcpy(data(), src, totalBytes);
    }

    uint8_t & operator()(uint32_t u, uint32_t v, uint32_t c)
    {
        return m_data[ (v*m_width + u)*m_channels +c ] ;//r.ptr[v*r.width+u];
    }
    uint8_t const & operator()(uint32_t u, uint32_t v, uint32_t c) const
    {
        return m_data[ (v*m_width + u)*m_channels +c ] ;//r.ptr[v*r.width+u];
    }
    ColorChannel& operator[](size_t i)
    {
        return (&r)[i];
    }
    ColorChannel const& operator[](size_t i) const
    {
        return (&r)[i];
    }
    /**
     * @brief sample
     * @param u
     * @param v
     * @return
     *
     * Samples a 2x2 block of pixels and returns the average. really only used for
     * the nextMipMap() method.
     */
    uint8_t sample(uint32_t u, uint32_t v, uint32_t c) const
    {
        auto & R = *this;

        auto c1 = R(u,v    ,c);
        auto c2 = R(u,v+1  ,c);
        auto c3 = R(u+1,v  ,c);
        auto c4 = R(u+1,v+1,c);

        return  static_cast<uint8_t>( (static_cast<uint32_t>(c1) + static_cast<uint32_t>(c2) + static_cast<uint32_t>(c3) + static_cast<uint32_t>(c4) ) / 4u);
    }

    /**
     * @brief nextMipMap
     * @return
     *
     * Returns the next mipmap level of the image. The next mipmap level has
     * width and height which is half the original.
     */
    Image nextMipMap() const
    {
        Image out;
        out.resize( getWidth()/2, getHeight()/2, getChannels());

        for(uint32_t j=0;j<out.getHeight();j++)
        {
            for(uint32_t i=0;i<out.getWidth();i++)
            {
                for(uint32_t c=0; c < out.getChannels(); c++)
                {
                    out(i,j,c) = sample( i*2, j*2,c );
                }
            }
        }
        return out;
    }

    Image allocateNextMipMap() const
    {
        Image out;
        out.resize( getWidth()/2, getHeight()/2, getChannels());
        return out;
    }

    void const* data() const
    {
        return m_data.data();
    }
    void * data()
    {
        return m_data.data();
    }
    size_t size() const
    {
        return m_data.size();
    }
    size_t byteSize() const
    {
        return m_data.size();
    }

    uint32_t width() const
    {
        return m_width;
    }
    uint32_t height() const
    {
        return m_height;
    }

    uint32_t getWidth() const
    {
        return m_width;
    }
    uint32_t getHeight() const
    {
        return m_height;
    }
    uint32_t getChannels() const
    {
        return m_channels;
    }
public:

    size_t hash() const
    {
        auto     w = getWidth();
        auto     h = getHeight();
        uint32_t c = 4;

        auto hashCo = [](size_t _seed, size_t h2)
        {
            _seed ^= h2 + 0x9e3779b9 + (_seed<<6) + (_seed>>2);
            return _seed;
        };

        std::hash<uint32_t> Hu;

        auto seed = Hu(w);
        seed = hashCo(seed, Hu(h) );
        seed = hashCo(seed, Hu(c) );

        auto * begin = static_cast<uint32_t const*>(data());
        for(uint32_t i=0;i<w*h;i++)
        {
            seed = hashCo(seed, Hu(*begin++));
        }
        return seed;
    }
//#endif

    std::vector<uint8_t> m_data;
    uint32_t             m_channels;
    uint32_t             m_width;
    uint32_t             m_height;
    ColorChannel r;
    ColorChannel g;
    ColorChannel b;
    ColorChannel a;

    /**
     * @brief X
     * @param width
     * @param height
     * @return
     *
     * Returns a oneD_Channel where the value of the channel increases
     * linearly in the u direction
     */
    static channel1f X(uint32_t width, uint32_t height)
    {
        channel1f D(width,height);

        float sc = 1.0f / float(width);
        for( uint32_t v=0; v< height; ++v)
        {
            for( uint32_t u=0; u<width; ++u)
            {
                float x = static_cast<float>(u) * sc;
                D(u,v) = x;
            }
        }
        return D;
    }

    /**
     * @brief Y
     * @param width
     * @param height
     * @return
     *
     * Returns a oneD_Channel where the value of the channel increases
     * linearly in the v direction
     */
    static channel1f Y(uint32_t width, uint32_t height)
    {
        channel1f D(width,height);

        float sc = 1.0f / float(height);
        for( uint32_t v=0; v< height; ++v)
        {
            float y = static_cast<float>(v) * sc;
            for( uint32_t u=0; u<width; ++u)
            {
                D(u,v) = y;
            }
        }
        return D;
    }
};



inline channel1f mix( ColorChannel const & a, ColorChannel const & b, float t)
{

    channel1f D( a.getWidth(), a.getHeight());

    auto * A = &a(0,0);
    auto * B = &b(0,0);

    float sc = 1.0f/255.0f;
    for(auto & v : D.data)
    {

        float x1 = static_cast<float>(*A);
        float x2 = static_cast<float>(*B);
        v   = ( (1.0f - t) * x1 + t * x2 ) * sc;

        A+=4; B+=4;
    }
    return D;
}

inline channel1f mix( ColorChannel const & a, ColorChannel const & b, ColorChannel const & _t)
{
    channel1f D( a.getWidth(), a.getHeight());

    auto * A = &a(0,0);
    auto * B = &b(0,0);
    auto * T = &_t(0,0);

    float sc = 1.0f/255.0f;

    for(auto & v : D.data)
    {
        float t = *T * sc;

        v  = ( (1.0f - t) * (*A) + t * (*B) ) * sc;

        A+=4; B+=4; T+=4;
    }
    return D;
}


inline Image mix( Image const & a, Image const & b, Image const & _t)
{
    Image D;
    assert( a.getWidth() == b.getWidth());
    assert( a.getHeight() == b.getHeight());
    assert( a.getChannels() == b.getChannels());

    assert( _t.getWidth() == b.getWidth());
    assert( _t.getHeight() == b.getHeight());
    assert( _t.getChannels() == b.getChannels());
    D.resize(a.getWidth(), a.getHeight(), a.getChannels());

    auto w = std::min( a.getWidth(),    b.getWidth()   );
    auto h = std::min( a.getHeight(),   b.getHeight()  );
    auto C = std::min( a.getChannels(), b.getChannels());


    for(uint32_t j=0;j<h;j++)
    {
        for(uint32_t i=0;i<w;i++)
        {
            for(uint32_t c =0; c < C; c++)
            {
                float t = static_cast<float>(_t(i,j,c)) / 255.0f;
                D(i,j,c) = mix( a(i,j,c), b(i,j,c), t );
            }
        }
    }

    return D;
}

inline Image mix( Image const & a, Image const & b, float t)
{
    Image D;
    assert( a.getWidth() == b.getWidth());
    assert( a.getHeight() == b.getHeight());
    assert( a.getChannels() == b.getChannels());

    D.resize(a.getWidth(), a.getHeight(), a.getChannels());

    auto w = std::min( a.getWidth(),    b.getWidth()   );
    auto h = std::min( a.getHeight(),   b.getHeight()  );
    auto C = std::min( a.getChannels(), b.getChannels());


    for(uint32_t j=0;j<h;j++)
    {
        for(uint32_t i=0;i<w;i++)
        {
            for(uint32_t c =0; c < C; c++)
            {
                D(i,j,c) = mix( a(i,j,c), b(i,j,c), t );
            }
        }
    }

    return D;
}

/**
 * @brief The ImageMM struct
 *
 * An array of Images that represent the
 * chain of mipmaps.
 */
struct ImageMM
{
    std::vector<Image> level;

    ImageMM()
    {
        level.resize(1);
    }


    Image & getLevel(size_t i)
    {
        return level.at(i);
    }
    Image const& getLevel(size_t i) const
    {
        return level.at(i);
    }
    void resize(uint32_t w, uint32_t h)
    {
        level[0].resize(w,h);
    }

    uint32_t getChannels() const
    {
        return level.front().getChannels();
    }
    uint32_t getHeight() const
    {
        return level.front().getHeight();
    }
    uint32_t getWidth() const
    {
        return level.front().getWidth();
    }
    uint32_t getLevelCount() const
    {
        return static_cast<uint32_t>(level.size());
    }

    uint32_t maxLevels() const
    {
        auto w =  level.at(0).getWidth();
        auto h =  level.at(0).getHeight();
        auto m =  uint32_t( std::log2( std::min(w,h) ) );
        return m;
    }

    void allocateMipMaps(uint32_t mips=0)
    {
        auto maxMips = maxLevels();

        if( mips != 0)
        {
            mips = std::min(maxMips, mips);
        }
        else
        {
            mips = maxMips;
        }
        mips=mips-1;
        level.resize(1);
        while(mips--)
        {
            auto m = level.back().allocateNextMipMap();
            level.push_back(m);
        }
    }
    void clearMipMaps()
    {
        level.resize(1);
    }
};

struct ImageArray
{
    std::vector<ImageMM> layer;

    ImageArray()
    {
        layer.resize(1);
    }

    ImageArray(Image const & I)
    {
        ImageMM mm;
        mm.level.clear();
        mm.level.push_back(I);
        layer.push_back( std::move(mm));
    }
    ImageArray(Image && I)
    {
        ImageMM mm;
        mm.level.clear();
        mm.level.push_back( std::move(I) );
        layer.push_back( std::move(mm));
    }

    ImageMM& getLayer(size_t i)
    {
        return layer.at(i);
    }
    ImageMM const& getLayer(size_t i) const
    {
        return layer.at(i);
    }

    void resize(uint32_t w, uint32_t h, uint32_t L=1, uint32_t mips=1)
    {
        layer.clear();
        layer.resize(L);
        for(auto & l : layer)
        {
            l.resize(w,h);
            l.allocateMipMaps(mips);
        }
    }
    uint32_t getChannels() const
    {
        return layer.front().getChannels();
    }
    uint32_t getHeight() const
    {
        return layer.front().getHeight();
    }
    uint32_t getWidth() const
    {
        return layer.front().getWidth();
    }
    uint32_t getLevelCount() const
    {
        return layer.front().getLevelCount();
    }
    uint32_t getLayerCount() const
    {
        return static_cast<uint32_t>(layer.size());
    }
    void allocateMipMaps(uint32_t mips=0)
    {
        for(auto & l : layer)
        {
            l.allocateMipMaps(mips);
        }
    }

};

}


namespace std
{
template<>
struct hash<gul::Image>
{
    static inline size_t hashCombine(size_t seed, size_t h2)
    {
        //std::hash<T> hasher;
        seed ^= h2 + 0x9e3779b9 + (seed<<6) + (seed>>2);
        return seed;
    }

    std::size_t operator()(gul::Image const & img) const noexcept
    {
        auto     w = img.getWidth();
        auto     h = img.getHeight();
        uint32_t c = img.getChannels();

        std::hash<uint32_t> Hu;

        auto seed = Hu(w);
        seed = hashCombine(seed, Hu(h) );
        seed = hashCombine(seed, Hu(c) );

        auto * begin = static_cast<uint32_t const*>(img.data());
        for(uint32_t i=0;i<w*h;i++)
        {
            seed = hashCombine(seed, Hu(*begin++));
        }
        return seed;
    }
};



}
#endif
