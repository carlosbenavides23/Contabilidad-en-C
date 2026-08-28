module financial_core
    use iso_c_binding
    implicit none

contains

    subroutine calculate_net_amounts(n, amounts, tax_rates, retentions, &
                                     net_amounts) &
        bind(C, name="financial_core_calculate_net_amounts")
        integer(c_int), value :: n
        real(c_double), intent(in) :: amounts(*)
        real(c_double), intent(in) :: tax_rates(*)
        real(c_double), intent(in) :: retentions(*)
        real(c_double), intent(out) :: net_amounts(*)

        if (n <= 0_c_int) return

        net_amounts(1:n) = amounts(1:n) * &
                           (1.0_c_double + tax_rates(1:n)) - &
                           amounts(1:n) * retentions(1:n)
    end subroutine calculate_net_amounts

end module financial_core
