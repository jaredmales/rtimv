/** \file rtimvLog.hpp
 * \brief Shared log prefix/message formatter helpers for rtimv apps.
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#ifndef rtimv_rtimvLog_hpp
#define rtimv_rtimvLog_hpp

#include <string>
#include <string_view>

namespace rtimv
{

/// Context used to format log prefixes/messages.
struct logContext
{
    std::string calledName;      ///< Program name, typically argv[0] basename.
    std::string image0;          ///< Primary image key/name for context.
    std::string clientId;        ///< Client id/peer, when available.
    bool includeAppName{ true }; ///< True to include called-name in prefix.
    bool includeClient{ false }; ///< True to include client id in prefix.
};

/// Format the standardized log prefix from context.
inline std::string formatLogPrefix( const logContext &ctx )
{
    std::string prefix;
    std::string fields;

    if( ctx.includeAppName && !ctx.calledName.empty() )
    {
        prefix += ctx.calledName + " (";
    }
    else
    {
        prefix += "(";
    }

    if( ctx.includeClient && !ctx.clientId.empty() )
    {
        fields += "client: " + ctx.clientId;
    }

    if( !ctx.image0.empty() )
    {
        if( !fields.empty() )
        {
            fields += " ";
        }

        fields += "image: " + ctx.image0;
    }

    prefix += fields;
    prefix += "): ";
    return prefix;
}

/// Format a full standardized log message from context and message text.
inline std::string formatLogMessage( const logContext &ctx, std::string_view message )
{
    std::string out = formatLogPrefix( ctx );
    out += message;
    return out;
}

} // namespace rtimv

#endif // rtimv_rtimvLog_hpp
