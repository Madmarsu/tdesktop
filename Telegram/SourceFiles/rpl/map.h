/*
This file is part of Telegram Desktop,
the official desktop version of Telegram messaging app, see https://telegram.org

Telegram Desktop is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

It is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

In addition, as a special exception, the copyright holders give permission
to link the code of portions of this program with the OpenSSL library.

Full license: https://github.com/telegramdesktop/tdesktop/blob/master/LICENSE
Copyright (c) 2014-2017 John Preston, https://desktop.telegram.org
*/
#pragma once

#include <rpl/producer.h>

namespace rpl {
namespace details {

template <
	typename Transform,
	typename NewValue,
	typename Error>
class map_transform_helper {
public:
	map_transform_helper(
		Transform &&transform,
		const consumer<NewValue, Error> &consumer)
		: _transform(std::move(transform))
		, _consumer(consumer) {
	}
	template <
		typename OtherValue,
		typename = std::enable_if_t<
			std::is_rvalue_reference_v<OtherValue&&>>>
	void operator()(OtherValue &&value) const {
		_consumer.put_next_forward(_transform(std::move(value)));
	}
	template <
		typename OtherValue,
		typename = decltype(
			std::declval<Transform>()(const_ref_val<OtherValue>()))>
	void operator()(const OtherValue &value) const {
		_consumer.put_next_forward(_transform(value));
	}

private:
	consumer<NewValue, Error> _consumer;
	Transform _transform;

};

template <
	typename Transform,
	typename NewValue,
	typename Error,
	typename = std::enable_if_t<
		std::is_rvalue_reference_v<Transform&&>>>
map_transform_helper<Transform, NewValue, Error> map_transform(
		Transform &&transform,
		const consumer<NewValue, Error> &consumer) {
	return { std::move(transform), consumer };
}

template <typename Transform>
class map_helper {
public:
	template <typename OtherTransform>
	map_helper(OtherTransform &&transform)
		: _transform(std::forward<OtherTransform>(transform)) {
	}

	template <
		typename Value,
		typename Error,
		typename NewValue = decltype(
			std::declval<Transform>()(std::declval<Value>())
		)>
	rpl::producer<NewValue, Error> operator()(
			rpl::producer<Value, Error> &&initial) {
		return [
			initial = std::move(initial),
			transform = std::move(_transform)
		](const consumer<NewValue, Error> &consumer) mutable {
			return std::move(initial).start(
			map_transform(
				std::move(transform),
				consumer
			), [consumer](auto &&error) {
				consumer.put_error_forward(
					std::forward<decltype(error)>(error));
			}, [consumer] {
				consumer.put_done();
			});
		};
	}

private:
	Transform _transform;

};

} // namespace details

template <typename Transform>
auto map(Transform &&transform)
-> details::map_helper<std::decay_t<Transform>> {
	return details::map_helper<std::decay_t<Transform>>(
		std::forward<Transform>(transform));
}

namespace details {

template <
	typename Transform,
	typename Value,
	typename NewError>
class map_error_transform_helper {
public:
	map_error_transform_helper(
		Transform &&transform,
		const consumer<Value, NewError> &consumer)
		: _transform(std::move(transform))
		, _consumer(consumer) {
	}
	template <
		typename OtherError,
		typename = std::enable_if_t<
			std::is_rvalue_reference_v<OtherError&&>>>
	void operator()(OtherError &&error) const {
		_consumer.put_error_forward(_transform(std::move(error)));
	}
	template <
		typename OtherError,
		typename = decltype(
			std::declval<Transform>()(const_ref_val<OtherError>()))>
	void operator()(const OtherError &error) const {
		_consumer.put_error_forward(_transform(error));
	}

private:
	consumer<Value, NewError> _consumer;
	Transform _transform;

};

template <
	typename Transform,
	typename Value,
	typename NewError,
	typename = std::enable_if_t<
		std::is_rvalue_reference_v<Transform&&>>>
map_error_transform_helper<Transform, Value, NewError>
map_error_transform(
		Transform &&transform,
		const consumer<Value, NewError> &consumer) {
	return { std::move(transform), consumer };
}

template <typename Transform>
class map_error_helper {
public:
	template <typename OtherTransform>
	map_error_helper(OtherTransform &&transform)
		: _transform(std::forward<OtherTransform>(transform)) {
	}

	template <
		typename Value,
		typename Error,
		typename NewError = decltype(
			std::declval<Transform>()(std::declval<Error>())
		)>
	rpl::producer<Value, NewError> operator()(
			rpl::producer<Value, Error> &&initial) {
		return [
			initial = std::move(initial),
			transform = std::move(_transform)
		](const consumer<Value, NewError> &consumer) mutable {
			return std::move(initial).start(
			[consumer](auto &&value) {
				consumer.put_next_forward(
					std::forward<decltype(value)>(value));
			}, map_error_transform(
				std::move(transform),
				consumer
			), [consumer] {
				consumer.put_done();
			});
		};
	}

private:
	Transform _transform;

};

} // namespace details

template <typename Transform>
auto map_error(Transform &&transform)
-> details::map_error_helper<std::decay_t<Transform>> {
	return details::map_error_helper<std::decay_t<Transform>>(
		std::forward<Transform>(transform));
}

} // namespace rpl
