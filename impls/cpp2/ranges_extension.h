#ifndef _MY_RANGES_EXTENSION_H_
#define _MY_RANGES_EXTENSION_H_

#include <ranges>
#include <tuple>
#include <utility>
#include <algorithm>
#include <iterator>

//
// ──────────────────────────────── ZIP VIEW ────────────────────────────────
//

namespace std::ranges {

    template <input_range... Views>
    requires (sizeof...(Views) > 0) && (view<Views> && ...)
    class zip_view : public view_interface<zip_view<Views...>> {
    private:
        tuple<Views...> views_;

        template <bool Const>
        class iterator {
            using Parent = conditional_t<Const, const zip_view, zip_view>;
            Parent* parent_;
            tuple<iterator_t<conditional_t<Const, const Views, Views>>...> current_;

        public:
            using iterator_concept =
                common_type_t<conditional_t<random_access_range<Views>, random_access_iterator_tag,
                              conditional_t<bidirectional_range<Views>, bidirectional_iterator_tag,
                              conditional_t<forward_range<Views>, forward_iterator_tag,
                              input_iterator_tag>>>...>;

            using iterator_category = input_iterator_tag; // legacy
            using value_type = tuple<range_value_t<conditional_t<Const, const Views, Views>>...>;
            using difference_type = ptrdiff_t;

            iterator() = default;

            iterator(Parent* parent,
                     tuple<iterator_t<conditional_t<Const, const Views, Views>>...> current)
                : parent_(parent), current_(std::move(current)) {}

            auto operator*() const {
                return apply([](auto&... it) {
                    return tuple<range_reference_t<decltype(it)>...>(*it...);
                }, current_);
            }

            iterator& operator++() {
                apply([](auto&... it) { (++it, ...); }, current_);
                return *this;
            }

            void operator++(int) { ++*this; }

            iterator& operator--()
            requires (bidirectional_range<Views> && ...)
            {
                apply([](auto&... it) { (--it, ...); }, current_);
                return *this;
            }

            iterator& operator+=(difference_type n)
            requires (random_access_range<Views> && ...)
            {
                apply([&](auto&... it) { ((it += n), ...); }, current_);
                return *this;
            }

            iterator& operator-=(difference_type n)
            requires (random_access_range<Views> && ...)
            {
                apply([&](auto&... it) { ((it -= n), ...); }, current_);
                return *this;
            }

            friend iterator operator+(iterator it, difference_type n)
            requires (random_access_range<Views> && ...)
            {
                it += n;
                return it;
            }

            friend iterator operator-(iterator it, difference_type n)
            requires (random_access_range<Views> && ...)
            {
                it -= n;
                return it;
            }

            friend difference_type operator-(const iterator& x, const iterator& y)
            requires (random_access_range<Views> && ...)
            {
                return min({ (get<iterator_t<conditional_t<Const, const Views, Views>>>(x.current_) -
                               get<iterator_t<conditional_t<Const, const Views, Views>>>(y.current_))... });
            }

            friend bool operator==(const iterator& x, const iterator& y)
            requires (equality_comparable<iterator_t<Views>> && ...)
            {
                return x.current_ == y.current_;
            }

            friend bool operator==(const iterator& it, const auto& sent) {
                bool reached_end = false;
                apply([&](auto&... its) {
                    reached_end = ((its == get<sentinel_t<Views>>(sent.ends_)) || ...);
                }, it.current_);
                return reached_end;
            }

            friend auto operator<=>(const iterator& x, const iterator& y)
            requires (random_access_range<Views> && ...)
            {
                return (get<0>(x.current_) <=> get<0>(y.current_));
            }
        };

        class sentinel {
        public:
            tuple<sentinel_t<Views>...> ends_;
            sentinel() = default;
            explicit sentinel(tuple<sentinel_t<Views>...> ends) : ends_(std::move(ends)) {}
        };

    public:
        zip_view() = default;
        explicit zip_view(Views... views) : views_(std::move(views)...) {}

        auto begin() {
            return iterator<false>(this, apply([](auto&... v) {
                return tuple(begin(v)...);
            }, views_));
        }

        auto end() {
            return sentinel(apply([](auto&... v) {
                return tuple(end(v)...);
            }, views_));
        }

        auto begin() const requires (range<const Views> && ...) {
            return iterator<true>(this, apply([](auto const&... v) {
                return tuple(begin(v)...);
            }, views_));
        }

        auto end() const requires (range<const Views> && ...) {
            return sentinel(apply([](auto const&... v) {
                return tuple(end(v)...);
            }, views_));
        }

        auto size() requires (sized_range<Views> && ...) {
            return apply([](auto&... v) {
                return min({ static_cast<size_t>(size(v))... });
            }, views_);
        }

        auto size() const requires (sized_range<const Views> && ...) {
            return apply([](auto const&... v) {
                return min({ static_cast<size_t>(size(v))... });
            }, views_);
        }
    };

    namespace views {
        struct zip_fn {
            // 1) direct-call: zip(a, b, c) -> zip_view
            template <viewable_range R, viewable_range... Rngs>
            auto operator()(R&& r, Rngs&&... rngs) const {
                return std::ranges::zip_view<views::all_t<R>, views::all_t<Rngs>...>(
                    views::all(std::forward<R>(r)),
                    views::all(std::forward<Rngs>(rngs))...);
            }

            // 2) adaptor type for pipe style (captures RHS ranges)
            template <viewable_range... Captured>
            struct adaptor {
                std::tuple<views::all_t<Captured>...> captured_;

                constexpr adaptor(std::tuple<views::all_t<Captured>...> t)
                    : captured_(std::move(t)) {}

                // apply adaptor to left-range -> produce a zip_view
                template <viewable_range R>
                auto operator()(R&& r) const {
                    return std::apply(
                        [&](auto const&... caps) {
                            return std::ranges::zip_view<views::all_t<R>, decltype(caps)...>(
                                views::all(std::forward<R>(r)), caps...);
                        },
                        captured_);
                }

                // allow "left | adaptor"
                template <class R>
                friend auto operator|(R&& r, adaptor const& a)
                    -> decltype(a(std::forward<R>(r))) {
                    return a(std::forward<R>(r));
                }
            };

            // 3) operator() that returns an adaptor when called as views::zip(b, c, ...)
            template <viewable_range... Rngs>
            auto operator()(Rngs&&... rngs) const {
                return adaptor<Rngs...>{
                    std::tuple<views::all_t<Rngs>...>(views::all(std::forward<Rngs>(rngs))...) };
            }
        };

        inline constexpr zip_fn zip{};
    }

} // namespace std::ranges

//
// ──────────────────────────────── CHUNK VIEW ────────────────────────────────
//

namespace std::ranges {

    template <view V>
    requires input_range<V>
    class chunk_view : public view_interface<chunk_view<V>> {
    private:
        V base_;
        range_difference_t<V> n_; // chunk size

        template <bool Const>
        class iterator {
            using Base = conditional_t<Const, const V, V>;
            iterator_t<Base> current_;
            sentinel_t<Base> end_;
            range_difference_t<Base> n_;

        public:
            using iterator_category = input_iterator_tag;
            using iterator_concept =
                conditional_t<random_access_range<Base>, random_access_iterator_tag,
                conditional_t<bidirectional_range<Base>, bidirectional_iterator_tag,
                conditional_t<forward_range<Base>, forward_iterator_tag,
                input_iterator_tag>>>;

            using value_type = subrange<iterator_t<Base>>;
            using difference_type = range_difference_t<Base>;

            iterator() = default;
            iterator(iterator_t<Base> first, sentinel_t<Base> last, difference_type n)
                : current_(first), end_(last), n_(n) {}

            auto operator*() const {
                auto first = current_;
                auto last = current_;
                for (difference_type i = 0; i < n_ && last != end_; ++i, ++last);
                return subrange(first, last);
            }

            iterator& operator++() {
                for (difference_type i = 0; i < n_ && current_ != end_; ++i, ++current_);
                return *this;
            }

            void operator++(int) { ++*this; }

            friend bool operator==(const iterator& x, const iterator& y)
            requires forward_range<Base>
            {
                return x.current_ == y.current_;
            }

            friend bool operator==(const iterator& it, sentinel_t<Base> s) {
                return it.current_ == s;
            }
        };

    public:
        chunk_view() = default;
        chunk_view(V base, range_difference_t<V> n) : base_(std::move(base)), n_(n) {}

        auto begin() {
            return iterator<false>(ranges::begin(base_), ranges::end(base_), n_);
        }

        auto end() {
            if constexpr (forward_range<V>)
                return iterator<false>(ranges::end(base_), ranges::end(base_), n_);
            else
                return ranges::end(base_);
        }

        auto begin() const requires range<const V> {
            return iterator<true>(ranges::begin(base_), ranges::end(base_), n_);
        }

        auto end() const requires range<const V> {
            if constexpr (forward_range<const V>)
                return iterator<true>(ranges::end(base_), ranges::end(base_), n_);
            else
                return ranges::end(base_);
        }

        auto size() requires sized_range<V> {
            auto sz = ranges::size(base_);
            return (sz + static_cast<decltype(sz)>(n_) - 1) / static_cast<decltype(sz)>(n_);
        }

        auto size() const requires sized_range<const V> {
            auto sz = ranges::size(base_);
            return (sz + static_cast<decltype(sz)>(n_) - 1) / static_cast<decltype(sz)>(n_);
        }
    };

    namespace views {
        struct chunk_fn {
            // direct call: views::chunk(r, n)
            template <viewable_range R>
            auto operator()(R&& r, range_difference_t<R> n) const {
                return chunk_view<views::all_t<R>>(views::all(std::forward<R>(r)), n);
            }

            // adaptor type for pipe-style: views::chunk(n)
            struct adaptor {
                range_difference_t<empty_view<int>> n_;
                constexpr adaptor(range_difference_t<empty_view<int>> n) : n_(n) {}

                template <viewable_range R>
                auto operator()(R&& r) const {
                    return chunk_view<views::all_t<R>>(views::all(std::forward<R>(r)), n_);
                }

                // make pipe work: left | adaptor -> adaptor(left)
                template <class R>
                friend auto operator|(R&& r, const adaptor& a)
                    -> decltype(a(std::forward<R>(r))) {
                    return a(std::forward<R>(r));
                }
            };

            // called as views::chunk(n)
            constexpr auto operator()(range_difference_t<empty_view<int>> n) const {
                return adaptor{n};
            }
        };

        inline constexpr chunk_fn chunk{};
    }

} // namespace std::ranges

#endif // _MY_RANGES_EXTENSION_H_
