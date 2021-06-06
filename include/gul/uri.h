#ifndef GUL_URI_H
#define GUL_URI_H

#include<regex>
#include<string>
#include<algorithm>
#include<stdexcept>
#include<algorithm>
#include<locale>

namespace gul
{
struct uri
{
    std::string scheme;

    std::string user;
    std::string password;
    std::string host;
    std::string port;

    std::string path;
    std::string query;
    std::string fragment;

    uri()
    {}

    explicit uri(const std::string &str)
    {
        parse(str);
    }

    uri& operator=(const std::string & str)
    {
        parse(str);
        return *this;
    }

    std::string toString() const
    {
        std::string out;
       if( scheme.size()!=0 ) out += scheme + ":";
       auto auth = getAuthority();
       if(auth.size())
       {
           out += "//" + auth;
       }

       out += path;
       if(query.size())
       {
           out += "?" + query;
       }
       if( fragment.size())
       {
           out += "#" + fragment;
       }
       return out;
    }
    std::string getAuthority() const
    {
        std::string out;
        if( user.size() )
        {
            out += user;
            if(password.size())
                out += ":" + password;
            out += "@";
        }
        out += host;
        if( port.size() )
            out += ":" + port;
        return out;
    }

    void parse(const std::string & str)
    {
        // This parsing code was taken from Facebook's Folly library
        // the Boost regex was replaced with std::regex
        /*
         * Copyright (c) Facebook, Inc. and its affiliates.
         *
         * Licensed under the Apache License, Version 2.0 (the "License");
         * you may not use this file except in compliance with the License.
         * You may obtain a copy of the License at
         *
         *     http://www.apache.org/licenses/LICENSE-2.0
         *
         * Unless required by applicable law or agreed to in writing, software
         * distributed under the License is distributed on an "AS IS" BASIS,
         * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
         * See the License for the specific language governing permissions and
         * limitations under the License.
         */

      static const std::regex uriRegex(
          "([a-zA-Z][a-zA-Z0-9+.-]*):" // scheme:
          "([^?#]*)" // authority and path
          "(?:\\?([^#]*))?" // ?query
          "(?:#(.*))?"); // #fragment
      static const std::regex authorityAndPathRegex("//([^/]*)(/.*)?");

      std::smatch match;
      if ( !std::regex_match(str, match, uriRegex))
      {
          throw std::invalid_argument( "invalid URI: " + str);
      }

      scheme = match[1];
      std::transform(scheme.begin(), scheme.end(), scheme.begin(),
          [](unsigned char c){ return std::tolower(c); });

      std::string authorityAndPath(match[2].first, match[2].second);
      std::smatch authorityAndPathMatch;
      if (!std::regex_match(
              authorityAndPath,
              authorityAndPathMatch,
              authorityAndPathRegex)) {
        // Does not start with //, doesn't have authority

        path = authorityAndPath;
      }
      else
      {
        static const std::regex authorityRegex(
            "(?:([^@:]*)(?::([^@]*))?@)?" // username, password
            "(\\[[^\\]]*\\]|[^\\[:]*)" // host (IP-literal (e.g. '['+IPv6+']',
                                       // dotted-IPv4, or named host)
            "(?::(\\d*))?"); // port

        std::string authority = authorityAndPathMatch[1];
        std::smatch authorityMatch;
        if (!std::regex_match(
                authority,
                authorityMatch,
                authorityRegex)) {
          throw std::invalid_argument( "invalid URI authority");
        }

        port = std::string(authorityMatch[4].first, authorityMatch[4].second);


        user     = authorityMatch[1];
        password = authorityMatch[2];
        host     = authorityMatch[3];
        path     = authorityAndPathMatch[2];
      }

      query    = match[3];//, 3);
      fragment = match[4];//, 4);
    }


    std::string_view getMediaEncoding() const
    {
        auto i = std::find( path.begin(), path.end(), ',');
        if( i == path.end() )
            return {};

        return std::string_view(&path[0], static_cast<size_t>(std::distance(path.begin(),i)) );
    }
    std::string_view getMediaType() const
    {
        auto i = std::find( path.begin(), path.end(), ';');
        if( i == path.end() )
            return {};
        auto j = std::find( i, path.end(), ',');
        if( j == path.end() )
            return {};
        ++i;
        return std::string_view(&(*i), static_cast<size_t>(std::distance(i,j)));
    }
    std::string_view getMediaData() const
    {
        auto i = getMediaEncoding();

        auto & f = path[ i.size()+1];
        auto & b = path.back();
        return std::string_view( &f, 1+static_cast<size_t>( std::distance(&f,&b)));
    }
};

}

#endif
