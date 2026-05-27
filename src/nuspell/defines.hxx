/* Copyright 2016-2024 Dimitrij Mijoski
 *
 * This file is part of Nuspell.
 *
 * Nuspell is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nuspell is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nuspell.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NUSPELL_DEFINES_HXX
#define NUSPELL_DEFINES_HXX

#define NUSPELL_EXPORT
#define NUSPELL_DEPRECATED_EXPORT

#define NUSPELL_BEGIN_INLINE_NAMESPACE namespace v5 {
#define NUSPELL_END_INLINE_NAMESPACE }

#undef NUSPELL_NODISCARD
#undef NUSPELL_DEPRECATED

#if __cplusplus >= 201703L
#define NUSPELL_NODISCARD [[nodiscard]]
#define NUSPELL_DEPRECATED [[deprecated]]
#else
#define NUSPELL_NODISCARD __attribute__((warn_unused_result))
#define NUSPELL_DEPRECATED __attribute__((deprecated))
#endif

#endif // NUSPELL_DEFINES_HXX
